#pragma once
#include <string>
#include <vector>

struct CameraConfig
{
    std::string device;
    std::string format;
    int width;
    int height;
    int fps;
};

struct BarcodeConfig
{
    std::vector<std::string> formats;
    bool draw_mode;
};
