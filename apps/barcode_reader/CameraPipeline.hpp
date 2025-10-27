#pragma once

#include "Config.hpp"
#include <opencv2/opencv.hpp>

class CameraPipeline
{
public:
    explicit CameraPipeline(const CameraConfig &config);

    bool ReadFrame(cv::Mat &frame);

    static void ListFormats(const char *device);
    static void ListFrameIntervals(const char *device, uint32_t pixfmt, uint32_t width, uint32_t height);
    void PrintActualSettings();

private:
    CameraConfig m_Config;
    cv::VideoCapture m_Cap;
};
