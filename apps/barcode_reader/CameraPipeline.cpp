#include "CameraPipeline.hpp"
#include <stdexcept>
#include <spdlog/spdlog.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <iostream>
#include <cstring>

CameraPipeline::CameraPipeline(const CameraConfig &config)
    : m_Config(config), m_Cap(config.device, cv::CAP_V4L2)
{
    if (!m_Cap.isOpened())
    {
        throw std::runtime_error("Could not open video source: " + config.device);
    }

    m_Cap.set(cv::CAP_PROP_FRAME_WIDTH, config.width);
    m_Cap.set(cv::CAP_PROP_FRAME_HEIGHT, config.height);
    m_Cap.set(cv::CAP_PROP_FPS, config.fps);

    if (config.format == "MJPG")
    {
        m_Cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
    }
    else if (config.format == "YUYV")
    {
        m_Cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('Y', 'U', 'Y', 'V'));
    }

    m_Cap.set(cv::CAP_PROP_FRAME_WIDTH, config.width);
    m_Cap.set(cv::CAP_PROP_FRAME_HEIGHT, config.height);
    m_Cap.set(cv::CAP_PROP_FPS, config.fps);

    PrintActualSettings();
}

bool CameraPipeline::ReadFrame(cv::Mat &frame)
{
    if (!m_Cap.isOpened())
    {
        SPDLOG_ERROR("Error: Camera is not opened");
        return false;
    }

    auto ret = m_Cap.read(frame);
    if (!ret)
    {
        SPDLOG_ERROR("Failed to read frame from camera");
        return false;
    }

    if (frame.empty())
    {
        SPDLOG_ERROR("Captured empty frame");
        return false;
    }
    return true;
}

void CameraPipeline::ListFormats(const char *device)
{
    int fd = open(device, O_RDWR);
    if (fd < 0)
    {
        SPDLOG_ERROR("Failed to open device: {}", device);
        return;
    }

    struct v4l2_fmtdesc fmt;
    std::memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    SPDLOG_INFO("Supported formats for device {}:", device);
    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmt) == 0)
    {
        std::string fourcc_str;
        fourcc_str += static_cast<char>(fmt.pixelformat & 0xFF);
        fourcc_str += static_cast<char>((fmt.pixelformat >> 8) & 0xFF);
        fourcc_str += static_cast<char>((fmt.pixelformat >> 16) & 0xFF);
        fourcc_str += static_cast<char>((fmt.pixelformat >> 24) & 0xFF);

        SPDLOG_INFO("  [{}] {} ({})",
                    fmt.index,
                    reinterpret_cast<const char *>(fmt.description),
                    fourcc_str);

        struct v4l2_frmsizeenum frmsize;
        frmsize.pixel_format = fmt.pixelformat;
        frmsize.index = 0;

        while (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0)
        {
            if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE)
            {
                SPDLOG_INFO("    Size: {}x{}", frmsize.discrete.width, frmsize.discrete.height);
            }
            frmsize.index++;
        }

        fmt.index++;
    }

    close(fd);
}

void CameraPipeline::ListFrameIntervals(const char *device, uint32_t pixfmt, uint32_t width, uint32_t height)
{
    int fd = open(device, O_RDWR);
    if (fd < 0)
    {
        SPDLOG_ERROR("Failed to open device: {}", device);
        return;
    }

    struct v4l2_frmivalenum frmival;
    std::memset(&frmival, 0, sizeof(frmival));
    frmival.pixel_format = pixfmt;
    frmival.width = width;
    frmival.height = height;

    SPDLOG_INFO("Supported frame intervals for {}x{}:", width, height);
    while (ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) == 0)
    {
        if (frmival.type == V4L2_FRMIVAL_TYPE_DISCRETE)
        {
            double fps = static_cast<double>(frmival.discrete.denominator) / frmival.discrete.numerator;
            SPDLOG_INFO("  {}/{} ({:.1f} fps)",
                        frmival.discrete.numerator, frmival.discrete.denominator, fps);
        }
        else if (frmival.type == V4L2_FRMIVAL_TYPE_STEPWISE)
        {
            SPDLOG_INFO("  Stepwise frame intervals (min: {}/{}, max: {}/{})",
                        frmival.stepwise.min.numerator, frmival.stepwise.min.denominator,
                        frmival.stepwise.max.numerator, frmival.stepwise.max.denominator);
        }
        frmival.index++;
    }

    close(fd);
}

void CameraPipeline::PrintActualSettings()
{
    if (!m_Cap.isOpened())
    {
        SPDLOG_ERROR("Camera not opened, cannot print settings");
        return;
    }

    double width = m_Cap.get(cv::CAP_PROP_FRAME_WIDTH);
    double height = m_Cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    double fps = m_Cap.get(cv::CAP_PROP_FPS);
    double fourcc = m_Cap.get(cv::CAP_PROP_FOURCC);

    std::string fourcc_str;
    fourcc_str += static_cast<char>(static_cast<int>(fourcc) & 0xFF);
    fourcc_str += static_cast<char>((static_cast<int>(fourcc) >> 8) & 0xFF);
    fourcc_str += static_cast<char>((static_cast<int>(fourcc) >> 16) & 0xFF);
    fourcc_str += static_cast<char>((static_cast<int>(fourcc) >> 24) & 0xFF);

    SPDLOG_INFO("Camera Settings:");
    SPDLOG_INFO("Requested: {}x{} @ {} fps, format: {}",
                m_Config.width, m_Config.height, m_Config.fps, m_Config.format);
    SPDLOG_INFO("Actual: {}x{} @ {:.1f} fps, fourcc: {}",
                static_cast<int>(width), static_cast<int>(height), fps, fourcc_str);
    SPDLOG_INFO("======================");
}
