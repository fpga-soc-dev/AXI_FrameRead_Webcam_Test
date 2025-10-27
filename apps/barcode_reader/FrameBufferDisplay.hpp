#pragma once

#include <opencv2/opencv.hpp>
#include <string>

class FrameBufferDisplay
{
public:
    explicit FrameBufferDisplay(const std::string &fb_device = "/dev/fb0");
    ~FrameBufferDisplay();

    FrameBufferDisplay(const FrameBufferDisplay &) = delete;
    FrameBufferDisplay &operator=(const FrameBufferDisplay &) = delete;

    bool Initialize();
    bool DisplayFrame(const cv::Mat &frame);
    void PrintInfo() const;

    cv::Size GetDisplaySize() const;
    int GetBitsPerPixel() const;

private:
    std::string m_FbDevice;
    int m_FbFd;
    void *m_FbMem;
    size_t m_FbSize;
    int m_Width;
    int m_Height;
    int m_BitsPerPixel;
    int m_LineLength;

    bool MapFrameBuffer();
    void UnmapFrameBuffer();
};