#include "OverlayRenderer.hpp"
#include <cmath>

OverlayRenderer::~OverlayRenderer() {
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
}

void OverlayRenderer::init(int viewportWidth, int viewportHeight) {
    m_width  = viewportWidth;
    m_height = viewportHeight;

    m_shader.load("shaders/overlay.vert", "shaders/overlay.frag");

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    // layout: [x(float), y(float), r(float), g(float), b(float)] = 5 floats
    constexpr GLsizei stride = 5 * sizeof(float);
    // position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);
    // colour
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void OverlayRenderer::threatColor(ThreatLevel lvl, float& r, float& g, float& b) {
    switch (lvl) {
        case ThreatLevel::HIGH:   r=1.f; g=0.f; b=0.f; break;
        case ThreatLevel::MEDIUM: r=1.f; g=1.f; b=0.f; break;
        default:                  r=0.f; g=1.f; b=0.f; break;
    }
}

void OverlayRenderer::pushLine(float x0, float y0,
                                float x1, float y1,
                                float r,  float g, float b) {
    // NDC conversion
    float nx0 = ndcX(x0), ny0 = ndcY(y0);
    float nx1 = ndcX(x1), ny1 = ndcY(y1);

    m_vertices.insert(m_vertices.end(),
        { nx0, ny0, r, g, b,
          nx1, ny1, r, g, b });
}

void OverlayRenderer::render(const TrackList& tracks) {
    m_vertices.clear();

    for (const auto& t : tracks) {
        if (t.state == TrackState::DEAD) continue;
        // Only draw ACTIVE and (optionally) LOST tracks
        if (t.state == TrackState::NEW    && t.activeFrameCount < 1) continue;

        float r, g, b;
        threatColor(t.threatLevel, r, g, b);

        // --- Bounding box (4 line segments) ---
        const cv::Rect& bb = t.boundingBox;
        float x0 = static_cast<float>(bb.x);
        float y0 = static_cast<float>(bb.y);
        float x1 = static_cast<float>(bb.x + bb.width);
        float y1 = static_cast<float>(bb.y + bb.height);

        pushLine(x0, y0, x1, y0, r, g, b); // top
        pushLine(x1, y0, x1, y1, r, g, b); // right
        pushLine(x1, y1, x0, y1, r, g, b); // bottom
        pushLine(x0, y1, x0, y0, r, g, b); // left

        // --- Velocity arrow ---
        const float ARROW_SCALE = 4.f;
        float cx = t.position.x;
        float cy = t.position.y;
        float ex = cx + t.velocity.x * ARROW_SCALE;
        float ey = cy + t.velocity.y * ARROW_SCALE;
        if (std::hypot(t.velocity.x, t.velocity.y) > 0.5f)
            pushLine(cx, cy, ex, ey, 0.f, 0.8f, 1.f);

        // --- Trail (line strip = N-1 segments) ---
        if (t.trail.size() >= 2) {
            for (size_t k = 0; k + 1 < t.trail.size(); ++k) {
                // Fade alpha by encoding brightness
                float fade = static_cast<float>(k) / static_cast<float>(t.trail.size());
                pushLine(t.trail[k].x,   t.trail[k].y,
                         t.trail[k+1].x, t.trail[k+1].y,
                         r * fade, g * fade, b * fade);
            }
        }
    }

    if (m_vertices.empty()) return;

    // Upload and draw
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_vertices.size() * sizeof(float)),
                 m_vertices.data(), GL_STREAM_DRAW);

    m_shader.use();

    // Enable alpha blending for nice overlay
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLsizei numVertices = static_cast<GLsizei>(m_vertices.size() / 5);
    glDrawArrays(GL_LINES, 0, numVertices);

    glDisable(GL_BLEND);
    glBindVertexArray(0);
}
