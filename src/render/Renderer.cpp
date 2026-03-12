#include "Renderer.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdexcept>
#include <string>

// ---- GLFW callback -------------------------------------------------------
void Renderer::framebufferSizeCallback(GLFWwindow* w, int width, int height) {
    glViewport(0, 0, width, height);
    // Update overlay viewport via user pointer
    auto* self = static_cast<Renderer*>(glfwGetWindowUserPointer(w));
    if (self) {
        self->m_winW = width;
        self->m_winH = height;
        self->m_overlay.setViewport(width, height);
    }
}

// ---- Init ----------------------------------------------------------------
void Renderer::init(int videoWidth, int videoHeight, const std::string& title) {
    m_winW = videoWidth;
    m_winH = videoHeight;

    if (!glfwInit())
        throw std::runtime_error("Failed to initialise GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(videoWidth, videoHeight, title.c_str(),
                                nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(0); // disable vsync for max throughput

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
        throw std::runtime_error("Failed to initialise GLAD");

    glViewport(0, 0, videoWidth, videoHeight);

    // ---- Shaders & GL objects ----
    m_videoShader.load("shaders/video.vert", "shaders/video.frag");
    m_videoTexture.init(videoWidth, videoHeight);
    m_overlay.init(videoWidth, videoHeight);

    setupQuad();

    // ---- Dear ImGui ----
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
}

// ---- Fullscreen quad geometry --------------------------------------------
void Renderer::setupQuad() {
    // NDC positions + UV: [x, y, u, v]
    static const float QUAD[] = {
        -1.f,  1.f,  0.f, 0.f,  // top-left
        -1.f, -1.f,  0.f, 1.f,  // bottom-left
         1.f, -1.f,  1.f, 1.f,  // bottom-right

        -1.f,  1.f,  0.f, 0.f,  // top-left
         1.f, -1.f,  1.f, 1.f,  // bottom-right
         1.f,  1.f,  1.f, 0.f   // top-right
    };

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);

    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD), QUAD, GL_STATIC_DRAW);

    constexpr GLsizei stride = 4 * sizeof(float);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

// ---- Destructor ----------------------------------------------------------
Renderer::~Renderer() {
    if (m_window) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        if (m_quadVBO) glDeleteBuffers(1, &m_quadVBO);
        if (m_quadVAO) glDeleteVertexArrays(1, &m_quadVAO);

        glfwDestroyWindow(m_window);
        glfwTerminate();
    }
}

// ---- Frame lifecycle -----------------------------------------------------
bool Renderer::isRunning() const {
    return m_window && !glfwWindowShouldClose(m_window);
}

bool Renderer::beginFrame() {
    glfwPollEvents();

    if (glfwWindowShouldClose(m_window)) return false;
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
        return false;
    }

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    // ImGui new frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    return true;
}

void Renderer::uploadFrame(const cv::Mat& bgrFrame) {
    m_videoTexture.upload(bgrFrame);
}

void Renderer::drawVideo() {
    m_videoShader.use();
    m_videoTexture.bind(0);
    m_videoShader.setInt("uTexture", 0);

    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Renderer::drawOverlays(const TrackList& tracks) {
    m_overlay.render(tracks);
}

void Renderer::drawHud(const TrackList& tracks, float fps, bool paused) {
    m_hud.render(tracks, fps, paused);
}

void Renderer::endFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(m_window);
}

bool Renderer::keyPressed(int glfwKey) const {
    return glfwGetKey(m_window, glfwKey) == GLFW_PRESS;
}
