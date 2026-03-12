#pragma once
#include "common/Types.hpp"
#include <opencv2/video/background_segm.hpp>
#include <opencv2/core.hpp>

// MOG2 background subtractor + contour-based detector.
// Returns a DetectionList for each frame.
class Detector {
public:
    Detector();

    // Process one BGR frame and return detected blobs.
    DetectionList detect(const cv::Mat& bgrFrame);

private:
    cv::Ptr<cv::BackgroundSubtractorMOG2> m_mog2;
    cv::Mat m_fgMask;       // reused each frame
    cv::Mat m_morphKernel;

    static constexpr float MIN_AREA = 500.f;
    static constexpr float MAX_AREA = 50000.f;
};
