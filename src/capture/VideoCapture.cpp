#include "VideoCapture.hpp"
#include <stdexcept>
#include <string>

VideoSource::VideoSource(const std::string& path) {
    m_cap.open(path);
    if (!m_cap.isOpened())
        throw std::runtime_error("VideoSource: cannot open \"" + path + "\"");

    m_width  = static_cast<int>(m_cap.get(cv::CAP_PROP_FRAME_WIDTH));
    m_height = static_cast<int>(m_cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    m_fps    = m_cap.get(cv::CAP_PROP_FPS);
    if (m_fps <= 0.0) m_fps = 30.0;
}

bool VideoSource::read(cv::Mat& dst) {
    return m_cap.read(dst);
}

bool VideoSource::isOpen() const {
    return m_cap.isOpened();
}
