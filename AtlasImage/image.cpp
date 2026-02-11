#include "image.hpp"

#include "AtlasLogger/atlaslogger.hpp"
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>


namespace AtlasImage
{

  static AtlasLogger::Logger m_logger{
    QStandardPaths::standardLocations(QStandardPaths::AppConfigLocation).at(1).toStdString() + 
    "/Atlas-Imager/Logs/" + AtlasLogger::GetCurrentDateString() + "/Image.log", 
    "AtlasImage::Image"
  };

  Image::Image(const std::string_view imageName) : m_imageName{imageName}
  {
    m_logger.Log(AtlasLogger::LogLevel::Info, "Image constructor called with: {}", imageName);
    m_image = std::make_shared<cv::Mat>(cv::imread(std::string{imageName}.c_str(), cv::IMREAD_GRAYSCALE));
    m_logger.Log(AtlasLogger::LogLevel::Info, "Image created with size: {}x{}", m_image->cols, m_image->rows);
  }

  auto Image::SetImage(const std::string_view imageName) -> void
  {
    m_image = std::make_shared<cv::Mat>(cv::imread(std::string{imageName}.c_str()));
  }

  auto Image::CloneData(const cv::Mat& toClone) -> void
  {
    m_image = std::make_shared<cv::Mat>(toClone.clone());
  }

  auto Image::GetImageName() const -> std::string_view
  {
    return m_imageName;
  }

  auto Image::GetImage() const -> const cv::Mat*
  {
    return m_image.get();
  }

  auto Image::GetRawData() const -> const unsigned char*
  {
    return m_image->data;
  }
}