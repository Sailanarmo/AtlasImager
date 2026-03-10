#include "image.hpp"

#include <opencv2/core.hpp>
#ifndef __EMSCRIPTEN__
#include <opencv2/highgui.hpp>
#endif

#include <print>

namespace AtlasImage
{
  Image::Image(const std::string_view imageName) : m_imageName{imageName}
  {
    std::println("Image constructor called with: {}", imageName);
#ifndef __EMSCRIPTEN__
    m_image = std::make_shared<cv::Mat>(cv::imread(std::string{imageName}.c_str(), cv::IMREAD_GRAYSCALE));
    std::println("Image created with size: {}x{}", m_image->cols, m_image->rows);
#else
    // In the WASM build cv::imread is not available — use the pixel-data
    // constructor instead. Default-construct an empty Mat as a safe no-op.
    m_image = std::make_shared<cv::Mat>();
    std::println("Image (WASM): path-based construction is a no-op; use pixel constructor.");
#endif
  }

#ifdef __EMSCRIPTEN__
  Image::Image(const std::string_view imageName,
               const unsigned char* data, int width, int height, int cvType)
    : m_imageName{imageName}
  {
    // Clone so the cv::Mat owns its buffer independently of the caller's memory.
    m_image = std::make_shared<cv::Mat>(
      cv::Mat(height, width, cvType, const_cast<unsigned char*>(data)).clone()
    );
    std::println("Image (WASM) created from raw pixels: {}x{}, cvType={}", width, height, cvType);
  }
#endif

  auto Image::SetImage(const std::string_view imageName) -> void
  {
#ifndef __EMSCRIPTEN__
    m_image = std::make_shared<cv::Mat>(cv::imread(std::string{imageName}.c_str()));
#else
    (void)imageName; // cv::imread not available in WASM build
#endif
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