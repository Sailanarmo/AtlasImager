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

#ifdef __EMSCRIPTEN__
      // WASM constructor: build directly from caller-provided raw pixel data.
      // data   – packed pixels (8-bit per channel, length = width*height*channels)
      // cvType – an OpenCV type constant, e.g. CV_8UC1 / CV_8UC3 / CV_8UC4
      Image(std::string_view imageName,
            const unsigned char* data, int width, int height, int cvType);
#endif

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