#include <chrono>
#include <thread>
#include <spdlog/spdlog.h>
#include "BarcodeProcessor.hpp"
#include "CameraPipeline.hpp"
#include "FrameBufferDisplay.hpp"

int main(int argc, char *argv[])
{
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("[%^%l%$] %v");

    FrameBufferDisplay display{"/dev/fb0"};
    if (!display.Initialize())
    {
        SPDLOG_ERROR("Error: Could not open framebuffer device");
        return -1;
    }

    display.PrintInfo();
    auto fb_resolution = display.GetDisplaySize();

    CameraConfig camera_config{
        "/dev/video0",
        "MJPG",
        640,
        480,
        30};

    CameraPipeline::ListFormats(camera_config.device.c_str());
    CameraPipeline::ListFrameIntervals(camera_config.device.c_str(),
                                       cv::VideoWriter::fourcc(camera_config.format[0], camera_config.format[1], camera_config.format[2], camera_config.format[3]),
                                       camera_config.width, camera_config.height);

    CameraPipeline camera{camera_config};

    BarcodeProcessor barcode_processor{ZXing::BarcodeFormat::Any};

    cv::Mat frame;
    int frame_count = 0;
    int barcode_frame_count = 0;
    const int BARCODE_SKIP = 3;
    auto start_time = std::chrono::steady_clock::now();

    while (true)
    {
        bool ret = camera.ReadFrame(frame);
        if (!ret)
        {
            SPDLOG_ERROR("Error: Failed to read frame from camera");
            break;
        }

        barcode_frame_count++;
        if (barcode_frame_count >= BARCODE_SKIP)
        {
            barcode_processor.RunBarcodeScanning(frame, true);
            barcode_frame_count = 0;
        }

        if (!display.DisplayFrame(frame))
        {
            SPDLOG_WARN("Failed to display frame to framebuffer");
        }

        frame_count++;
        if (frame_count % 150 == 0)
        {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time);
            double fps = frame_count / static_cast<double>(elapsed.count());
            SPDLOG_INFO("Processed {} frames, Average FPS: {:.1f}", frame_count, fps);
        }
    }

    return 0;
}