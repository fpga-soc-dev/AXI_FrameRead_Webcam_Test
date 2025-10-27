#include "BarcodeProcessor.hpp"
#include <spdlog/spdlog.h>

BarcodeProcessor::BarcodeProcessor(ZXing::BarcodeFormat format)
    : m_DetectorEngine(format), m_Running(true), m_IsRead(false)
{
    m_ProcessThread = std::thread(&BarcodeProcessor::ProcessThread, this);
}

BarcodeProcessor::~BarcodeProcessor()
{
    StopProcessing();
}

void BarcodeProcessor::ProcessBarcode(const BarcodeData &barcode_data)
{
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (m_LastBarcodeText == barcode_data.data)
        {
            return;
        }
        m_LastBarcodeText = barcode_data.data;
        m_LatestBarcode = barcode_data;
    }
    m_Condition.notify_one();
}

void BarcodeProcessor::StopProcessing()
{
    if (!m_Running.load())
    {
        return;
    }

    m_Running.store(false);
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_LatestBarcode.reset();
    }

    m_Condition.notify_one();
    if (m_ProcessThread.joinable())
    {
        m_ProcessThread.join();
    }
}

void BarcodeProcessor::ProcessThread()
{
    while (m_Running.load())
    {
        std::unique_lock<std::mutex> lock(m_Mutex);
        m_Condition.wait(lock, [this]
                         { return m_LatestBarcode.has_value() || !m_Running.load(); });

        if (!m_Running.load())
        {
            break;
        }

        if (m_LatestBarcode.has_value())
        {
            BarcodeData barcode_copy = *m_LatestBarcode;
            m_LatestBarcode.reset();
            lock.unlock();

            SPDLOG_INFO("Barcode: {} ({:.1f}ms)", barcode_copy.data, barcode_copy.processing_time.count());

            m_IsRead.store(true);
        }
    }
}

bool BarcodeProcessor::IsBarcodeRead() const
{
    return m_IsRead.load();
}

void BarcodeProcessor::ResetBarcodeFlag()
{
    m_IsRead.store(false);
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_LastBarcodeText.clear();
}

void BarcodeProcessor::RunBarcodeScanning(cv::Mat &frame, const bool draw_mode)
{
    if (frame.empty())
    {
        return;
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    auto barcodes = m_DetectorEngine.ReadBarcodes(frame);

    for (const auto &barcode : barcodes)
    {
        if (barcode.format() != ZXing::BarcodeFormat::None && barcode.isValid())
        {
            if (draw_mode)
            {
                m_DetectorEngine.DrawBarcode(frame, barcode);
            }

            auto end_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end_time - start_time;

            ProcessBarcode({barcode.text(), elapsed});

            break;
        }
    }
}