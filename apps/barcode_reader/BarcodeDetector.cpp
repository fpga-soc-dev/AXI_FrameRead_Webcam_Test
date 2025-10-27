#include "BarcodeDetector.hpp"

BarcodeDetector::BarcodeDetector(ZXing::BarcodeFormat format)
    : m_Format(format)
{
    m_Options.setFormats(m_Format);
    // PERFORMANCE OPTIMIZATIONS
    m_Options.setTryHarder(false);      // Disable slow deep scanning
    m_Options.setTryRotate(false);      // Disable rotation detection
    m_Options.setTryInvert(false);      // Disable inversion detection
    m_Options.setMaxNumberOfSymbols(1); // Stop after first barcode found

    if (format == ZXing::BarcodeFormat::Any)
    {
        m_Options.setFormats(ZXing::BarcodeFormat::QRCode |
                             ZXing::BarcodeFormat::Code128 |
                             ZXing::BarcodeFormat::Code39);
    }
}

ZXing::ImageView BarcodeDetector::ImageViewFromMat(const cv::Mat &image)
{

    auto fmt = ZXing::ImageFormat::None;
    switch (image.channels())
    {
    case 1:
        fmt = ZXing::ImageFormat::Lum;
        break;
    case 2:
        fmt = ZXing::ImageFormat::LumA;
        break;
    case 3:
        fmt = ZXing::ImageFormat::BGR;
        break;
    case 4:
        fmt = ZXing::ImageFormat::BGRA;
        break;
    }

    if (image.depth() != CV_8U || fmt == ZXing::ImageFormat::None)
    {
        return {nullptr, 0, 0, ZXing::ImageFormat::None};
    }

    return {image.data, image.cols, image.rows, fmt};
}

ZXing::Barcodes BarcodeDetector::ReadBarcodes(const cv::Mat &image)
{
    return ZXing::ReadBarcodes(ImageViewFromMat(image), m_Options);
}

void BarcodeDetector::DrawBarcode(cv::Mat &image, const ZXing::Barcode &barcode)
{
    auto pos = barcode.position();
    auto zx2cv = [](ZXing::PointI p)
    {
        return cv::Point(p.x, p.y);
    };

    auto contour = std::vector<cv::Point>{
        zx2cv(pos[0]),
        zx2cv(pos[1]),
        zx2cv(pos[2]),
        zx2cv(pos[3])};

    const auto *pts = contour.data();
    int npts = contour.size();

    cv::polylines(image, &pts, &npts, 1, true, CV_RGB(255, 0, 0), 3);

    cv::putText(image,
                barcode.text(),
                zx2cv(pos[3]) + cv::Point(0, 20),
                cv::FONT_HERSHEY_DUPLEX,
                1.0,
                CV_RGB(255, 0, 0));
}