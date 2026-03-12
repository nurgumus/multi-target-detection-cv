#pragma once
#include <string>
#include <opencv2/videoio.hpp>
#include <opencv2/core.hpp>

// RAII wrapper around cv::VideoCapture.
// Non-copyable; move semantics not needed for this project scope.
class VideoSource {
public:
    explicit VideoSource(const std::string& path);
    ~VideoSource() = default;

    // Non-copyable, but movable (needed to reassign in main loop)
    VideoSource(const VideoSource&)            = delete;
    VideoSource& operator=(const VideoSource&) = delete;
    VideoSource(VideoSource&&)                 = default;
    VideoSource& operator=(VideoSource&&)      = default;

    // Read next frame into dst. Returns false at end-of-stream.
    bool read(cv::Mat& dst);

    int    width()  const { return m_width;  }
    int    height() const { return m_height; }
    double fps()    const { return m_fps;    }
    bool   isOpen() const;

private:
    cv::VideoCapture m_cap;
    int    m_width  = 0;
    int    m_height = 0;
    double m_fps    = 30.0;
};
