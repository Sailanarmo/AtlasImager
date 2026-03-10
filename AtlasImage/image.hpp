#pragma once

#include <memory>
#include <string>
#include <string_view>

namespace cv
{
  class Mat;
}

/*
* This class is a wrapper for OpenCV Images. This wraps the OpenCV image object in a shared pointer
* that is then passed around when modifications are needed.
*/

namespace AtlasImage
{
  class Image
  {
    public:
      Image(const std::string_view imageName);

#ifdef __EMSCRIPTEN__
      // WASM constructor: build directly from caller-provided raw pixel data.
      // data   – packed pixels (8-bit per channel, length = width*height*channels)
      // cvType – an OpenCV type constant, e.g. CV_8UC1 / CV_8UC3 / CV_8UC4
      Image(std::string_view imageName,
            const unsigned char* data, int width, int height, int cvType);
#endif

      auto SetImage(const std::string_view imageName) -> void;
      auto CloneData(const cv::Mat& toClone) -> void;

      auto GetImageName() const -> std::string_view;
      auto GetImage() const -> const cv::Mat*;
      auto GetRawData() const -> const unsigned char*;

    
    private:
      std::shared_ptr<cv::Mat> m_image;
      const std::string m_imageName;
  };
}