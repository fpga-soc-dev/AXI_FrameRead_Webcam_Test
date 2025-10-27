#include "FrameBufferDisplay.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <spdlog/spdlog.h>
#include <cstring>

FrameBufferDisplay::FrameBufferDisplay(const std::string &fb_device)
    : m_FbDevice(fb_device), m_FbFd(-1), m_FbMem(nullptr),
      m_FbSize(0), m_Width(0), m_Height(0), m_BitsPerPixel(0), m_LineLength(0)
{
}

FrameBufferDisplay::~FrameBufferDisplay()
{
    UnmapFrameBuffer();
    if (m_FbFd >= 0)
    {
        close(m_FbFd);
    }
}

bool FrameBufferDisplay::Initialize()
{
    m_FbFd = open(m_FbDevice.c_str(), O_RDWR);
    if (m_FbFd < 0)
    {
        SPDLOG_ERROR("Cannot open framebuffer device: {}", m_FbDevice);
        return false;
    }

    // Get framebuffer info
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;

    if (ioctl(m_FbFd, FBIOGET_VSCREENINFO, &vinfo) < 0 ||
        ioctl(m_FbFd, FBIOGET_FSCREENINFO, &finfo) < 0)
    {
        SPDLOG_ERROR("Failed to get framebuffer info");
        close(m_FbFd);
        m_FbFd = -1;
        return false;
    }

    m_Width = vinfo.xres;
    m_Height = vinfo.yres;
    m_BitsPerPixel = vinfo.bits_per_pixel;
    m_LineLength = finfo.line_length;
    m_FbSize = finfo.smem_len;

    return MapFrameBuffer();
}

bool FrameBufferDisplay::DisplayFrame(const cv::Mat &frame)
{
    if (!m_FbMem || frame.empty())
    {
        return false;
    }

    if (frame.cols == m_Width && frame.rows == m_Height)
    {
        if (frame.isContinuous())
        {
            size_t copy_size = std::min(static_cast<size_t>(frame.total() * frame.elemSize()), m_FbSize);
            std::memcpy(m_FbMem, frame.data, copy_size);
        }
        else
        {
            const size_t line_bytes = frame.cols * frame.elemSize();
            for (int y = 0; y < frame.rows; ++y)
            {
                std::memcpy(static_cast<char *>(m_FbMem) + y * m_LineLength,
                            frame.ptr(y), line_bytes);
            }
        }
    }
    else if (frame.cols <= m_Width && frame.rows <= m_Height)
    {
        const int x_offset = (m_Width - frame.cols) / 2;
        const int y_offset = (m_Height - frame.rows) / 2;
        const size_t line_bytes = frame.cols * frame.elemSize();
        const size_t pixel_size = frame.elemSize();

        char *fb_start = static_cast<char *>(m_FbMem) + y_offset * m_LineLength + x_offset * pixel_size;

        for (int y = 0; y < frame.rows; ++y)
        {
            std::memcpy(fb_start + y * m_LineLength, frame.ptr(y), line_bytes);
        }
    }
    else
    {
        SPDLOG_ERROR("Frame size {}x{} larger than framebuffer {}x{}",
                     frame.cols, frame.rows, m_Width, m_Height);
        return false;
    }

    return true;
}

cv::Size FrameBufferDisplay::GetDisplaySize() const
{
    return cv::Size(m_Width, m_Height);
}

int FrameBufferDisplay::GetBitsPerPixel() const
{
    return m_BitsPerPixel;
}

bool FrameBufferDisplay::MapFrameBuffer()
{
    m_FbMem = mmap(nullptr, m_FbSize, PROT_READ | PROT_WRITE, MAP_SHARED, m_FbFd, 0);
    if (m_FbMem == MAP_FAILED)
    {
        SPDLOG_ERROR("Cannot memory map framebuffer (size: {} MB)", m_FbSize / (1024 * 1024));
        m_FbMem = nullptr;
        return false;
    }

    std::memset(m_FbMem, 0, m_FbSize);
    SPDLOG_INFO("Framebuffer mapped successfully");
    return true;
}

void FrameBufferDisplay::UnmapFrameBuffer()
{
    if (m_FbMem && m_FbMem != MAP_FAILED)
    {
        munmap(m_FbMem, m_FbSize);
        m_FbMem = nullptr;
    }
}

void FrameBufferDisplay::PrintInfo() const
{
    SPDLOG_INFO("=== Framebuffer Information ===");
    SPDLOG_INFO("Device: {}", m_FbDevice);
    SPDLOG_INFO("Resolution: {}x{}", m_Width, m_Height);
    SPDLOG_INFO("Bits per pixel: {}", m_BitsPerPixel);
    SPDLOG_INFO("Line length: {} bytes", m_LineLength);
    SPDLOG_INFO("Buffer size: {:.1f} MB", static_cast<double>(m_FbSize) / (1024 * 1024));
    SPDLOG_INFO("===============================");
}