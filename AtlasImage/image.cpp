#include "image.hpp"

#include "AtlasLogger/atlaslogger.hpp"

#include <opencv2/imgproc.hpp>

#ifndef __EMSCRIPTEN__
#include <opencv2/highgui.hpp>
#endif

namespace AtlasImage
{

#ifdef __EMSCRIPTEN__
  static AtlasLogger::Logger m_logger{"", "AtlasImage::Image"};
#else
  static AtlasLogger::Logger m_logger{
    QStandardPaths::standardLocations(QStandardPaths::AppConfigLocation).at(1).toStdString() +
    "/Atlas-Imager/Logs/" + AtlasLogger::GetCurrentDateString() + "/Image.log",
    "AtlasImage::Image"
  };
#endif

  Image::Image(const std::string_view imagePath, const bool loadInColor) : m_imageName{imagePath}
  {
    m_logger.Log(AtlasLogger::LogLevel::Info, "Image constructor called with: {}", imagePath);
    if(loadInColor)
    #ifndef __EMSCRIPTEN__
      m_image = cv::imread(std::string{imagePath}.c_str());
    else
      m_image = cv::imread(std::string{imagePath}.c_str(), cv::IMREAD_GRAYSCALE);
    #else
      m_image = cv::Mat{}; // In WASM, we construct empty and fill from raw pixel data (no cv::imread)
    #endif

    m_logger.Log(AtlasLogger::LogLevel::Info, "Image created with size: {}x{}", m_image.cols, m_image.rows);
  }

#ifdef __EMSCRIPTEN__
  Image::Image(const std::string_view imageName,
               const unsigned char* data, int width, int height, int cvType)
    : m_imageName{imageName}
  {
    // Clone so the cv::Mat owns its buffer independently of the caller's memory.
    m_image = cv::Mat(height, width, cvType, const_cast<unsigned char*>(data));
    m_logger.Log(AtlasLogger::LogLevel::Info, "Image (WASM) created from raw pixels: {}x{}, cvType={}", width, height, cvType);
  }
#endif

  Image::~Image()
  {
    m_logger.Log(AtlasLogger::LogLevel::Info, "Image destructor called for: {}", m_imageName);
  }

  auto Image::SetImage(const std::string_view imagePath) -> void
  {
    #ifndef __EMSCRIPTEN__
    m_image = cv::imread(std::string{imagePath}.c_str());
    #else
    (void)imagePath; // No-op in WASM, we don't support cv::imread
    m_logger.Log(AtlasLogger::LogLevel::Warning, "SetImage called with path: {} but cv::imread is not supported in WASM. No action taken.", imagePath);
    #endif
  }

  auto Image::GetImageName() const -> std::string_view
  {
    return m_imageName;
  }

  auto Image::GetImage() const -> const cv::Mat*
  {
    return &m_image;
  }

  auto Image::GetRawData() const -> const unsigned char*
  {
    return m_image.data;
  }

  auto Image::GetTransparentImage(const TransparentColorTarget target) const -> cv::Mat
  {
    m_logger.Log(AtlasLogger::LogLevel::Info, "Creating transparent image from: {}", m_imageName);
    cv::Mat transparentImage;
    cv::cvtColor(m_image, transparentImage, cv::ColorConversionCodes::COLOR_BGR2BGRA);

    for(auto y = 0; y < transparentImage.rows; ++y)
    {
      for(auto x = 0; x < transparentImage.cols; ++x)
      {
        auto& pixel = transparentImage.at<cv::Vec4b>(y, x);
        const int maxChannel = std::max({static_cast<int>(pixel[0]), static_cast<int>(pixel[1]), static_cast<int>(pixel[2])});
        if(maxChannel == static_cast<int>(target))
        {
          pixel[3] = 0; // Set alpha to 0 for fully transparent
        }
      }
    }

    return transparentImage;
  }
}