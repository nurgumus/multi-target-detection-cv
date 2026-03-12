#pragma once
#include <opencv2/video/tracking.hpp>
#include <opencv2/core.hpp>

// Per-target Kalman filter with constant-velocity model.
// State: [x, y, vx, vy]   Measurement: [x, y]
class KalmanTracker {
public:
    explicit KalmanTracker(cv::Point2f initialPos);

    // Advance state estimate by one frame (call before matching).
    cv::Point2f predict();

    // Incorporate a measurement after successful assignment.
    cv::Point2f correct(cv::Point2f measuredPos);

    cv::Point2f getPosition() const;
    cv::Point2f getVelocity() const;

private:
    cv::KalmanFilter m_kf;

    static constexpr int STATE_DIM = 4;  // x, y, vx, vy
    static constexpr int MEAS_DIM  = 2;  // x, y
};
