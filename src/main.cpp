#include <iostream>
#include <string>
#include <chrono>

#include "capture/VideoCapture.hpp"
#include "detection/Detector.hpp"
#include "tracking/TrackManager.hpp"
#include "threat/ThreatScorer.hpp"
#include "render/Renderer.hpp"
#include "common/Types.hpp"

// Simple FPS counter using an exponential moving average
class FpsCounter {
public:
    void tick() {
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - m_last).count();
        m_last = now;
        if (dt > 0.f)
            m_fps = m_fps * 0.9f + (1.f / dt) * 0.1f;
    }
    float fps() const { return m_fps; }
private:
    std::chrono::steady_clock::time_point m_last = std::chrono::steady_clock::now();
    float m_fps = 0.f;
};

int main(int argc, char** argv) {
    // ---- Argument parsing ----
    std::string videoPath;
    if (argc >= 2) {
        videoPath = argv[1];
    } else {
        std::cerr << "Usage: " << argv[0] << " <video_file>\n"
                  << "Example: CVTrackingSystem assets/sample_video.mp4\n";
        return 1;
    }

    // ---- Pipeline objects ----
    VideoSource  capture(videoPath);
    Detector     detector;
    TrackManager trackManager;
    ThreatScorer threatScorer;
    Renderer     renderer;
    FpsCounter   fpsCounter;

    renderer.init(capture.width(), capture.height(),
                  "CV Tracking System — " + videoPath);

    cv::Mat frame;
    bool paused = false;

    // ---- Main loop ----
    while (renderer.isRunning()) {
        // Handle pause (Space) — re-check key each frame
        static bool spaceWasDown = false;
        bool spaceNow = renderer.keyPressed(GLFW_KEY_SPACE);
        if (spaceNow && !spaceWasDown) paused = !paused;
        spaceWasDown = spaceNow;

        // Handle reset (R key)
        static bool rWasDown = false;
        bool rNow = renderer.keyPressed(GLFW_KEY_R);
        if (rNow && !rWasDown) {
            trackManager = TrackManager();
            detector     = Detector();
        }
        rWasDown = rNow;

        if (!paused) {
            if (!capture.read(frame)) {
                // End of file — loop by reopening
                capture = VideoSource(videoPath);
                trackManager = TrackManager();
                detector     = Detector();
                if (!capture.read(frame)) break;
            }

            // ---- Detection ----
            DetectionList detections = detector.detect(frame);

            // ---- Tracking ----
            trackManager.update(detections);
            TrackList tracks = trackManager.getTracks();  // copy for render

            // ---- Threat scoring ----
            threatScorer.score(tracks);

            // ---- Render ----
            if (!renderer.beginFrame()) break;

            renderer.uploadFrame(frame);
            renderer.drawVideo();
            renderer.drawOverlays(tracks);
            renderer.drawHud(tracks, fpsCounter.fps(), paused);

            renderer.endFrame();
            fpsCounter.tick();
        } else {
            // Paused: still need to service window events and render ImGui
            if (!renderer.beginFrame()) break;

            renderer.drawVideo();  // show last frame
            renderer.drawOverlays(trackManager.getTracks());
            renderer.drawHud(trackManager.getTracks(), fpsCounter.fps(), paused);
            renderer.endFrame();
        }
    }

    return 0;
}
