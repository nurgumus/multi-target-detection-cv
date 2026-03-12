# Real-Time Multi-Target Detection & Tracking System

A C++ computer vision pipeline that detects and tracks multiple moving objects in real time — from a webcam or video file — and renders them in an OpenGL window with color-coded threat overlays and a live HUD.

Built as a defense industry portfolio piece, demonstrating: sensor pipelines, Kalman filtering, data association, and real-time rendering discipline.

---

## Quick Start

```powershell
# Webcam (recommended to try first)
.\build\Release\CVTrackingSystem.exe 0

# Video file
.\build\Release\CVTrackingSystem.exe assets\sample_video.mp4
```

| Key | Action |
|-----|--------|
| `ESC` | Quit |
| `Space` | Pause / Resume |
| `R` | Reset tracker (clear all tracks) |

---

## What You See on Screen

```
┌─────────────────────────────────────────────────────────┐
│  ┌────────────────────┐                                  │
│  │ FPS: 47.2  Tgts: 3 │  ← ImGui HUD panel              │
│  ├────┬───────┬───────┤                                  │
│  │ ID │ Speed │Threat │                                  │
│  │  1 │ 5.2   │  MED  │                                  │
│  │  2 │ 11.0  │  HIGH │                                  │
│  │  3 │ 1.1   │  LOW  │                                  │
│  └────┴───────┴───────┘                                  │
│                                                          │
│         ┌──────────┐  ← red box = HIGH threat            │
│         │  TARGET  │───►  cyan arrow = velocity          │
│         └──────────┘                                     │
│        ·····  ← fading trail (last 30 positions)         │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

**Box colors** reflect threat level:

| Color | Threat | Condition |
|-------|--------|-----------|
| Green | LOW | Slow and small |
| Yellow | MEDIUM | Moderate speed or size |
| Red | HIGH | Fast and/or large |

---

## Architecture Overview

```mermaid
graph TD
    A[Camera / Video File] -->|BGR frame| B[VideoSource]
    B -->|cv::Mat| C[Detector]
    C -->|DetectionList| D[TrackManager]
    D -->|predict| E[KalmanTracker x N]
    E -->|predicted positions| D
    D -->|IoU cost matrix| F[Hungarian Algorithm]
    F -->|assignments| D
    D -->|correct + FSM update| E
    D -->|TrackList| G[ThreatScorer]
    G -->|mutates threatLevel| H[Renderer]
    B -->|raw frame| H
    H --> I[VideoTexture\nBGR→RGB upload]
    H --> J[OverlayRenderer\nGL_LINES boxes/arrows/trails]
    H --> K[HudPanel\nDear ImGui table]
    I --> L[OpenGL Window]
    J --> L
    K --> L
```

---

## Per-Frame Data Flow

Every frame goes through exactly this sequence — no exceptions:

```mermaid
sequenceDiagram
    participant Main
    participant VideoSource
    participant Detector
    participant TrackManager
    participant ThreatScorer
    participant Renderer

    Main->>VideoSource: read(frame)
    VideoSource-->>Main: cv::Mat (BGR)

    Main->>Detector: detect(frame)
    Note over Detector: MOG2 background subtraction<br/>→ threshold → morphological open<br/>→ findContours → area filter
    Detector-->>Main: DetectionList

    Main->>TrackManager: update(detections)
    Note over TrackManager: 1. Predict all Kalman filters<br/>2. Build IoU cost matrix<br/>3. Run Hungarian algorithm<br/>4. Correct matched filters<br/>5. FSM state transitions<br/>6. Spawn / prune tracks
    TrackManager-->>Main: TrackList

    Main->>ThreatScorer: score(tracks)
    Note over ThreatScorer: speed + area → LOW/MED/HIGH
    ThreatScorer-->>Main: (mutates tracks in-place)

    Main->>Renderer: uploadFrame + draw
    Note over Renderer: BGR→RGB → GL texture<br/>GL_LINES overlays<br/>ImGui HUD
    Renderer-->>Main: glfwSwapBuffers
```

---

## Module Deep-Dives

### 1. VideoSource (`capture/`)

A thin RAII wrapper around `cv::VideoCapture`. **RAII** (Resource Acquisition Is Initialization) means the camera is opened in the constructor and automatically closed when the object is destroyed — no manual cleanup needed.

```
VideoSource created
    └── constructor opens camera/file
    └── stores width, height, fps

VideoSource destroyed
    └── cv::VideoCapture destructor closes device automatically
```

Key C++ concept used: **deleted copy, defaulted move**

```cpp
VideoSource(const VideoSource&)        = delete;   // can't copy a camera handle
VideoSource& operator=(const VideoSource&) = delete;
VideoSource(VideoSource&&)             = default;  // CAN transfer ownership
VideoSource& operator=(VideoSource&&)  = default;  // used in the loop-reset
```

---

### 2. Detector (`detection/`)

Separates moving foreground from static background using **MOG2** (Mixture of Gaussians v2), a statistical background model built into OpenCV.

```mermaid
flowchart LR
    A[BGR Frame] --> B[MOG2\nBackground Subtractor]
    B --> C[Foreground Mask\n0=background 255=foreground\n127=shadow]
    C --> D[Threshold > 200\nRemove shadow pixels]
    D --> E[Morphological Open\nRemove tiny noise blobs]
    E --> F[Dilate\nFill gaps inside objects]
    F --> G[findContours]
    G --> H{Area filter\n500 – 50 000 px²}
    H -->|pass| I[DetectionList]
    H -->|reject| J[discard]
```

**What is morphological opening?**
It's an erosion followed by a dilation. Erosion shrinks blobs, removing tiny specks. Dilation grows them back. The net effect: small noise disappears, larger real objects survive.

Each surviving contour becomes a `Detection` struct:

```cpp
struct Detection {
    cv::Rect    boundingBox;   // rectangle around the blob
    cv::Point2f center;        // (x, y) center of that rectangle
    float       area;          // pixel area of the contour
};
```

---

### 3. KalmanTracker (`tracking/`)

One `KalmanTracker` lives inside `TrackManager` for each tracked target. It estimates where a target is *even when the detector misses it* for a few frames.

#### The State Vector

The filter tracks four numbers per target:

```
state = [ x,  y,  vx,  vy ]
          ↑   ↑    ↑    ↑
        pos  pos  vel  vel
         x    y    x    y
```

#### Constant-Velocity Model

The filter assumes targets move in a straight line between frames (constant velocity). The **transition matrix** encodes this:

```
next_state = F × current_state

[ x' ]   [ 1  0  1  0 ] [ x  ]
[ y' ] = [ 0  1  0  1 ] [ y  ]
[ vx']   [ 0  0  1  0 ] [ vx ]
[ vy']   [ 0  0  0  1 ] [ vy ]

  x' = x + vx   ← position advances by velocity
  y' = y + vy
  vx'= vx        ← velocity assumed constant
  vy'= vy
```

#### Predict → Correct Cycle

```mermaid
flowchart LR
    A[Start of frame\ncall predict] --> B[Filter projects state\nforward by 1 frame]
    B --> C{Was this target\nmatched to a detection?}
    C -->|Yes| D[call correct\nwith measured center]
    D --> E[Filter blends\nprediction + measurement]
    C -->|No| F[Use prediction only\nlostFrameCount++]
```

The brilliant part: when an object is briefly occluded (hidden behind something), the filter keeps predicting its position based on its last known velocity. The track stays alive for up to 15 missed frames.

---

### 4. TrackManager (`tracking/`)

This is the brain of the system. It decides which detection belongs to which track each frame.

#### The Assignment Problem

Imagine 3 tracks and 4 detections. You need to find the best 1-to-1 pairing. This is called the **assignment problem**.

```mermaid
graph LR
    subgraph Tracks
        T1[Track 1]
        T2[Track 2]
        T3[Track 3]
    end
    subgraph Detections
        D1[Det A]
        D2[Det B]
        D3[Det C]
        D4[Det D]
    end
    T1 -->|IoU=0.72| D2
    T2 -->|IoU=0.61| D3
    T3 -->|IoU=0.08| D1
    D4 -.->|unmatched → new track| D4
```

**IoU (Intersection over Union)** measures how much two rectangles overlap:

```
        area of overlap
IoU = ─────────────────────
       area of their union

IoU = 0.0  → no overlap at all
IoU = 1.0  → perfect overlap
```

We build an **N×M cost matrix** where `cost[i][j] = 1 - IoU(track_i, detection_j)`, then find the minimum-cost assignment using the **Hungarian algorithm**.

#### Track FSM (Finite State Machine)

Every track lives in one of four states:

```mermaid
stateDiagram-v2
    [*] --> NEW : detection with no match

    NEW --> ACTIVE : matched ≥ 3 frames in a row
    NEW --> DEAD : lost ≥ 15 frames

    ACTIVE --> LOST : missed 1 frame
    ACTIVE --> DEAD : lost ≥ 15 frames

    LOST --> ACTIVE : re-matched to a detection
    LOST --> DEAD : lost ≥ 15 frames

    DEAD --> [*]
```

**Why the NEW state?** A single spurious detection (a shadow, a reflection) shouldn't immediately become a track. It must survive 3 consecutive frames before being promoted to ACTIVE and shown in the HUD.

---

### 5. ThreatScorer (`threat/`)

Simple rule-based classifier. Stateless (no member variables), so it's trivially thread-safe.

```mermaid
flowchart TD
    A[Track] --> B{speed ≥ 8 px/frame?}
    B -->|yes| C[speedScore = 2]
    B -->|no| D{speed ≥ 3 px/frame?}
    D -->|yes| E[speedScore = 1]
    D -->|no| F[speedScore = 0]

    A --> G{area ≥ 15000 px²?}
    G -->|yes| H[areaScore = 2]
    G -->|no| I{area ≥ 5000 px²?}
    I -->|yes| J[areaScore = 1]
    I -->|no| K[areaScore = 0]

    C & E & F --> L[combined = speedScore + areaScore]
    H & J & K --> L

    L --> M{combined ≥ 3?}
    M -->|yes| N[HIGH 🔴]
    M -->|no| O{combined ≥ 1?}
    O -->|yes| P[MEDIUM 🟡]
    O -->|no| Q[LOW 🟢]
```

---

### 6. Renderer (`render/`)

The rendering layer owns the GLFW window and drives three sub-systems each frame.

#### OpenGL Rendering Pipeline

```mermaid
flowchart TD
    A[cv::Mat BGR frame] --> B[VideoTexture::upload\ncv::cvtColor BGR→RGB\nglTexSubImage2D]
    B --> C[ShaderProgram: video.vert/frag\nFullscreen quad\nGL_TRIANGLES 6 verts]
    C --> D[Video appears in window]

    E[TrackList] --> F[OverlayRenderer::render\nBuild vertex buffer\nx y r g b per vertex]
    F --> G[ShaderProgram: overlay.vert/frag\nGL_LINES\nboxes + arrows + trails]
    G --> H[Overlays drawn on top]

    I[TrackList + FPS] --> J[HudPanel::render\nDear ImGui table]
    J --> K[ImGui::Render\nImGui_ImplOpenGL3_RenderDrawData]
    K --> L[HUD drawn on top]

    D & H & L --> M[glfwSwapBuffers\nFrame shown to user]
```

#### Why BGR→RGB?

OpenCV stores images in **BGR** order (Blue, Green, Red) — a historical quirk from when it was first written. OpenGL expects **RGB**. Without the conversion, red and blue channels swap and everything looks wrong (faces appear blue-tinted, etc.).

```cpp
cv::cvtColor(bgrFrame, m_rgbBuf, cv::COLOR_BGR2RGB);  // mandatory!
```

#### Coordinate Systems

The overlay renderer converts pixel coordinates to **NDC** (Normalized Device Coordinates) that OpenGL understands:

```
Pixel space          NDC space
(0,0)──────────►    (-1,+1)──────►(+1,+1)
  │  (320,240)         │    (0,0)
  │                    │
  ▼(640,480)        (-1,-1)──────►(+1,-1)

ndcX = pixel_x / width  * 2 - 1
ndcY = 1 - pixel_y / height * 2   ← Y is flipped!
```

The Y flip is because pixels count downward from the top, but NDC counts upward from the bottom.

---

## File Map

```
cv-project/
│
├── CMakeLists.txt              Build system — links OpenCV, GLFW, GLAD, GLM, ImGui
├── vcpkg.json                  Dependency manifest for vcpkg
│
├── shaders/
│   ├── video.vert / .frag      Fullscreen textured quad
│   └── overlay.vert / .frag    Per-vertex color lines (boxes, arrows, trails)
│
├── assets/
│   └── (drop your .mp4 here)
│
└── src/
    ├── main.cpp                Main loop: read → detect → track → score → render
    │
    ├── common/
    │   ├── Types.hpp           Detection, Track, ThreatLevel, TrackState
    │   └── Hungarian.hpp       Header-only O(n³) assignment algorithm
    │
    ├── capture/
    │   └── VideoCapture        RAII camera/file wrapper
    │
    ├── detection/
    │   └── Detector            MOG2 + morphology + contour filter
    │
    ├── tracking/
    │   ├── KalmanTracker       [x, y, vx, vy] constant-velocity Kalman filter
    │   └── TrackManager        IoU matrix → Hungarian → FSM lifecycle
    │
    ├── threat/
    │   └── ThreatScorer        Speed + area → LOW / MEDIUM / HIGH
    │
    └── render/
        ├── Renderer            Top-level: owns GLFW window, drives loop
        ├── ShaderProgram       Compile + link GLSL shaders, RAII handle
        ├── VideoTexture        BGR→RGB, glTexSubImage2D upload
        ├── OverlayRenderer     GL_LINES scratch-buffer renderer
        └── HudPanel            Dear ImGui target table
```

---

## Build from Scratch

### Prerequisites

| Tool | Purpose |
|------|---------|
| Visual Studio 2022 | C++ compiler (MSVC) |
| CMake ≥ 3.21 | Build system |
| vcpkg | C++ package manager |

### Steps

```powershell
# 1. Install all dependencies (one-time, takes a few minutes)
cd C:\Users\pc\Desktop\cv-project
vcpkg install --triplet x64-windows

# 2. Configure the build
cmake -B build -S . `
  -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-windows

# 3. Compile
cmake --build build --config Release

# 4. Run
.\build\Release\CVTrackingSystem.exe 0              # webcam
.\build\Release\CVTrackingSystem.exe assets\vid.mp4 # video file
```

CMake automatically copies the shader files and all required OpenCV DLLs next to the `.exe` after each build.

---

## C++ Concepts Demonstrated

| Concept | Where |
|---------|-------|
| RAII (constructor acquires, destructor releases) | `VideoSource`, `ShaderProgram`, `VideoTexture` |
| Deleted copy / defaulted move | `VideoSource`, `Renderer` |
| `cv::Ptr<>` (OpenCV's smart pointer) | `Detector` — `BackgroundSubtractorMOG2` |
| `std::unique_ptr` for polymorphism | `TrackManager` — one `KalmanTracker*` per track |
| `enum class` (scoped, type-safe enum) | `ThreatLevel`, `TrackState` |
| `std::deque` (double-ended queue) | `Track::trail` — efficient front pop |
| Header-only template algorithm | `Hungarian.hpp` |
| `const` correctness | `ThreatScorer::score(const Track&)` |
| `std::clamp` | `ThreatScorer` — keeps score in `[0,4]` |
| Scratch buffer pattern | `OverlayRenderer::m_vertices` — pre-allocated, cleared each frame |

---

## Performance Notes

- The overlay vertex buffer is rebuilt from scratch each frame but never `new`/`delete`-allocated — `std::vector::clear()` keeps capacity, so no heap allocation in the hot path.
- `glTexSubImage2D` re-uses GPU texture storage allocated once in `init()` — no per-frame GPU alloc.
- `glfwSwapInterval(0)` disables vsync so the loop runs as fast as the hardware allows. On a typical laptop you should see 40–90 FPS at 640×480.
- The Hungarian algorithm is O(n³) in track count — fine for dozens of targets, would need approximate methods (e.g. auction algorithm) for hundreds.

---

## Extending the Project

Some ideas if you want to keep learning:

| Idea | Concepts learned |
|------|-----------------|
| Replace MOG2 with a YOLO model (`cv::dnn`) | Neural network inference in OpenCV |
| Add a config file (JSON/TOML) for thresholds | File I/O, serialization |
| Record output video with `cv::VideoWriter` | OpenCV encoding pipeline |
| Multi-thread detection + render on separate threads | `std::thread`, `std::mutex` |
| Replace Hungarian with JPDA for ghost tracks | Probabilistic data association |
| Add a map view (bird's-eye) as a second window | Multiple GL contexts |
