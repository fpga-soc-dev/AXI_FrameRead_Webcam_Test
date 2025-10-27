#pragma once

#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <optional>
#include <opencv2/opencv.hpp>
#include "BarcodeDetector.hpp"

class BarcodeProcessor
{
public:
    struct BarcodeData
    {
        std::string data;
        std::chrono::duration<double, std::milli> processing_time;
    };

    explicit BarcodeProcessor(ZXing::BarcodeFormat format);

    BarcodeProcessor(const BarcodeProcessor &) = delete;
    BarcodeProcessor &operator=(const BarcodeProcessor &) = delete;
    BarcodeProcessor(BarcodeProcessor &&) noexcept = default;
    BarcodeProcessor &operator=(BarcodeProcessor &&) noexcept = default;
    ~BarcodeProcessor();

    bool IsBarcodeRead() const;
    void RunBarcodeScanning(cv::Mat &frame, const bool draw_mode);
    void ResetBarcodeFlag();

private:
    void ProcessThread();
    void ProcessBarcode(const BarcodeData &barcode_data);
    void StopProcessing();

    BarcodeDetector m_DetectorEngine;
    std::thread m_ProcessThread;
    std::mutex m_Mutex;
    std::condition_variable m_Condition;
    std::atomic<bool> m_Running;
    std::atomic<bool> m_IsRead;
    std::optional<BarcodeData> m_LatestBarcode;
    std::string m_LastBarcodeText;
};