#include <ZXing/BarcodeFormat.h>
#include <ZXing/ReaderOptions.h>
#include <ZXing/ReadBarcode.h>
#include <opencv2/opencv.hpp>

class BarcodeDetector
{
public:
    explicit BarcodeDetector(ZXing::BarcodeFormat format);

    BarcodeDetector(const BarcodeDetector &) = delete;
    BarcodeDetector &operator=(const BarcodeDetector &) = delete;
    BarcodeDetector(BarcodeDetector &&) = default;
    BarcodeDetector &operator=(BarcodeDetector &&) = default;

    ZXing::ImageView ImageViewFromMat(const cv::Mat &image);
    ZXing::Barcodes ReadBarcodes(const cv::Mat &image);

    static void DrawBarcode(cv::Mat &image, const ZXing::Barcode &barcode);

private:
    ZXing::ReaderOptions m_Options;
    ZXing::BarcodeFormat m_Format;
};
