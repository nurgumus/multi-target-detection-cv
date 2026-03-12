#pragma once
#include <glad/glad.h>
#include <opencv2/core.hpp>

// Manages a GL_TEXTURE_2D that receives frames from OpenCV.
// Handles BGR→RGB conversion and row-alignment.
class VideoTexture {
public:
    VideoTexture() = default;
    ~VideoTexture();

    VideoTexture(const VideoTexture&)            = delete;
    VideoTexture& operator=(const VideoTexture&) = delete;

    // Call after GL context is current.
    void init(int width, int height);

    // Upload a BGR frame to the GPU.
    void upload(const cv::Mat& bgrFrame);

    void bind(int unit = 0) const;
    GLuint id() const { return m_texId; }

private:
    GLuint m_texId  = 0;
    int    m_width  = 0;
    int    m_height = 0;
    cv::Mat m_rgbBuf; // reused each frame to avoid per-frame alloc
};
