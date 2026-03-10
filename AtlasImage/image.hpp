#pragma once

#include <string>
#include <string_view>
#include <opencv2/core.hpp>

/*
* This class is a wrapper for OpenCV Images. This wraps the OpenCV image object in a shared pointer
* that is then passed around when modifications are needed.
*/

namespace AtlasImage
{
  enum class TransparentColorTarget : std::uint8_t
  {
    Black = 0,
    White = 255
  };
  class Image
  {
    public:
      Image(const std::string_view imagePath, const bool loadInColor = false);
      ~Image();

      auto SetImage(const std::string_view imagePath) -> void;

      auto GetImageName() const -> std::string_view;
      auto GetImage() const -> const cv::Mat*;
      auto GetRawData() const -> const unsigned char*;
      auto GetTransparentImage(const TransparentColorTarget target) const -> cv::Mat;

    
    private:
      cv::Mat m_image;
      const std::string m_imageName;
  };
}