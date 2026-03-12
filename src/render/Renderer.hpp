#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include "ShaderProgram.hpp"
#include "VideoTexture.hpp"
#include "OverlayRenderer.hpp"
#include "HudPanel.hpp"
#include "common/Types.hpp"

// Top-level renderer. Owns the GLFW window and drives the render loop.
// Non-copyable.
class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;

    // Create GLFW window, init GLAD, set up all GL objects.
    void init(int videoWidth, int videoHeight, const std::string& title);

    // Returns true if the window should stay open.
    bool isRunning() const;

    // Call each frame to begin a new render pass.
    // Returns false if the window was closed or ESC pressed.
    bool beginFrame();

    // Upload the latest video frame.
    void uploadFrame(const cv::Mat& bgrFrame);

    // Draw the fullscreen video quad.
    void drawVideo();

    // Draw track overlays.
    void drawOverlays(const TrackList& tracks);

    // Draw ImGui HUD.
    void drawHud(const TrackList& tracks, float fps, bool paused);

    // Swap buffers and poll events.
    void endFrame();

    // Keyboard query helpers (checked in main loop)
    bool keyPressed(int glfwKey) const;

    int windowWidth()  const { return m_winW; }
    int windowHeight() const { return m_winH; }

private:
    GLFWwindow*    m_window  = nullptr;
    ShaderProgram  m_videoShader;
    VideoTexture   m_videoTexture;
    OverlayRenderer m_overlay;
    HudPanel       m_hud;

    // Fullscreen quad
    GLuint m_quadVAO = 0;
    GLuint m_quadVBO = 0;

    int m_winW = 0;
    int m_winH = 0;

    void setupQuad();

    static void framebufferSizeCallback(GLFWwindow* w, int width, int height);
};
