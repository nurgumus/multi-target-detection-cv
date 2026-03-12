#include "VideoTexture.hpp"
#include <opencv2/imgproc.hpp>
#include <stdexcept>

VideoTexture::~VideoTexture() {
    if (m_texId) glDeleteTextures(1, &m_texId);
}

void VideoTexture::init(int width, int height) {
    m_width  = width;
    m_height = height;

    glGenTextures(1, &m_texId);
    glBindTexture(GL_TEXTURE_2D, m_texId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Allocate GPU storage
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void VideoTexture::upload(const cv::Mat& bgrFrame) {
    if (bgrFrame.empty()) return;

    // BGR → RGB (required — GL expects RGB)
    cv::cvtColor(bgrFrame, m_rgbBuf, cv::COLOR_BGR2RGB);

    // Ensure the data is contiguous in memory
    if (!m_rgbBuf.isContinuous())
        m_rgbBuf = m_rgbBuf.clone();

    glBindTexture(GL_TEXTURE_2D, m_texId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                    m_rgbBuf.cols, m_rgbBuf.rows,
                    GL_RGB, GL_UNSIGNED_BYTE,
                    m_rgbBuf.data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void VideoTexture::bind(int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_texId);
}
