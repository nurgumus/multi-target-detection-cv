#include "Detector.hpp"
#include <opencv2/imgproc.hpp>

Detector::Detector()
    // varThreshold: how much a pixel must change to be called foreground.
    // 16 = very sensitive (noisy), 36-50 = calmer, 64+ = only clear motion.
    : m_mog2(cv::createBackgroundSubtractorMOG2(500, 64, true))
    , m_morphKernel(cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)))
{}

DetectionList Detector::detect(const cv::Mat& bgrFrame) {
    // Background subtraction → foreground mask
    m_mog2->apply(bgrFrame, m_fgMask);

    // Remove shadows (shadow pixels are marked 127 by MOG2)
    cv::threshold(m_fgMask, m_fgMask, 200, 255, cv::THRESH_BINARY);

    // Morphological opening: remove small noise blobs
    cv::morphologyEx(m_fgMask, m_fgMask, cv::MORPH_OPEN, m_morphKernel);
    // Dilation to fill small gaps within objects
    cv::dilate(m_fgMask, m_fgMask, m_morphKernel, cv::Point(-1,-1), 2);

    // Find contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(m_fgMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    DetectionList detections;
    detections.reserve(contours.size());

    for (const auto& contour : contours) {
        float area = static_cast<float>(cv::contourArea(contour));
        if (area < MIN_AREA || area > MAX_AREA)
            continue;

        cv::Rect bb = cv::boundingRect(contour);
        Detection d;
        d.boundingBox = bb;
        d.center = cv::Point2f(bb.x + bb.width  * 0.5f,
                               bb.y + bb.height * 0.5f);
        d.area = area;
        detections.push_back(d);
    }
    return detections;
}
