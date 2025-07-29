#pragma once

#include "AtlasCommon/atlasenums.hpp"
#include "AtlasImage/image.hpp"

#include <array>
#include <memory>
#include <string>
#include <vector>
#include <string_view>

/*
* Atlas Model is the class that does the heavy lifting. This attempts to find the best 3 matches
* using OpenCV's BFMatcher (I believe this stands for Brute Force). It will then return the filename
* along with their scores. 
*
* TODO: This needs to be tested to see if the best matches are indeed the best matches. It is highly
*       possible that it is trying to match the entire image against the atlas model selected. If an
*       area of interest needs to be selected that work will need to be passed from Qt to here somehow.
*/
namespace AtlasModel
{
  class Model
  {
    public:
      Model();

      // auto GetBestFits(const std::string_view imageName) -> std::array<std::string, 3>;
      auto GetBestFits(const std::string_view imageName) -> std::vector<std::string>;
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