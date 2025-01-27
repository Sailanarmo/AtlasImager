#pragma once

#include "AtlasCommon/atlasenums.hpp"
#include "AtlasImage/image.hpp"

#include <array>
#include <memory>
#include <string>
#include <vector>
#include <string_view>
#include <atomic>

namespace AtlasModel
{
  class Model
  {
    public:
      Model(std::atomic<bool>& shut_down);

      auto GetBestFits(const std::string_view imageName) -> std::array<AtlasImage::Image, 5>;
      auto LoadDataSet(const AtlasCommon::AtlasDataSet dataSet) -> void;

      auto HandleMessage(const char* message) -> void;
    
    private:
      std::atomic<bool>& m_shutDown;
      static const std::unordered_map<AtlasCommon::AtlasDataSet, std::string> m_dataSetPaths; 

      std::vector<std::unique_ptr<AtlasImage::Image>> m_images;
      std::unique_ptr<AtlasImage::Image> m_imageToProcess{nullptr};

      auto InitializeModel() -> void;
      auto CalculateMatchScore() const -> double;
      
  };
}