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

      auto GetBestFits(const std::string_view imageName) -> std::array<std::string, 3>;
      auto LoadDataSet(const AtlasCommon::AtlasDataSet dataSet) -> void;

      auto HandleMessage(const char* message) -> void;
    
    private:
      static const std::unordered_map<AtlasCommon::AtlasDataSet, std::string> m_dataSetPaths; 

      std::vector<AtlasImage::Image> m_images;

      auto InitializeModel() -> void;
      auto GetQueryDescriptors(const AtlasImage::Image& img) -> AtlasImage::Image;
      auto CalculateMatchScore(const AtlasImage::Image& targetDescriptors, 
                               const AtlasImage::Image& modelDescriptors) -> std::pair<std::string, double>;
      
  };
}