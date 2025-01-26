#pragma once

#include "AtlasCommon/atlasenums.hpp"
#include "AtlasImage/image.hpp"

#include <array>
#include <memory>
#include <string>
#include <vector>
#include <string_view>


namespace AtlasModel
{
  class Model
  {
    public:
      Model();

      auto GetBestFits(const std::string_view imageName) const -> std::array<AtlasImage::Image, 5>;
      auto LoadDataSet(const AtlasCommon::AtlasDataSet dataSet) -> void;

      auto HandleMessage(const char* message) -> void;
    
    private:
      static const std::unordered_map<AtlasCommon::AtlasDataSet, std::string> m_dataSetPaths; 

      std::vector<std::unique_ptr<AtlasImage::Image>> m_images;

      auto InitializeModel() -> void;
      
  };
}