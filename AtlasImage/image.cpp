#include "image.hpp"

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include <print>

namespace AtlasImage
{
  Image::Image(const std::string_view imageName) : m_imageName{imageName}
  {
    std::println("Image constructor called with: {}", imageName);
    m_image = std::make_shared<cv::Mat>(cv::imread(std::string{imageName}.c_str()));
    std::println("Image created with size: {}x{}", m_image->cols, m_image->rows);
  }

  auto Image::SetImage(const std::string_view imageName) -> void
  {
    m_image = std::make_unique<cv::Mat>(cv::imread(std::string{imageName}.c_str()));
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