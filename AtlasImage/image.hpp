#pragma once

#include <memory>
#include <string>
#include <string_view>

namespace cv
{
  class Mat;
}

namespace AtlasImage
{
  class Image
  {
    public:
      Image(const std::string_view imageName);

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