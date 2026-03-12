#pragma once
#include <glad/glad.h>
#include "common/Types.hpp"
#include "ShaderProgram.hpp"

// Renders GL_LINES overlays: bounding boxes, velocity arrows, trails.
// Owns one VAO/VBO; rebuilds vertex data each frame from scratch.
class OverlayRenderer {
public:
    OverlayRenderer() = default;
    ~OverlayRenderer();

    OverlayRenderer(const OverlayRenderer&)            = delete;
    OverlayRenderer& operator=(const OverlayRenderer&) = delete;

    // Call after GL context is current.
    void init(int viewportWidth, int viewportHeight);

    // Re-upload geometry and draw all overlays for the given tracks.
    void render(const TrackList& tracks);

    // Call when window is resized.
    void setViewport(int w, int h) { m_width = w; m_height = h; }

private:
    ShaderProgram m_shader;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    int m_width  = 1;
    int m_height = 1;

    // Scratch buffer: [x, y, r, g, b] per vertex — avoids per-frame alloc
    std::vector<float> m_vertices;

    // Convert pixel coordinates to NDC
    float ndcX(float px) const { return px / m_width  * 2.f - 1.f; }
    float ndcY(float py) const { return 1.f - py / m_height * 2.f; }

    // Append a single line segment (two vertices)
    void pushLine(float x0, float y0, float x1, float y1,
                  float r, float g, float b);

    // Threat-level color
    static void threatColor(ThreatLevel lvl, float& r, float& g, float& b);
};
