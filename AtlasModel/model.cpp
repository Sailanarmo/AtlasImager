#include "model.hpp"
#include "AtlasMessenger/atlasmessenger.hpp"

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/xfeatures2d.hpp>

#include <thread>
#include <ranges>
#include <algorithm>
#include <filesystem>

#include <print>

namespace AtlasModel
{
  const std::unordered_map<AtlasCommon::AtlasDataSet, std::string> Model::m_dataSetPaths =
  {
    {AtlasCommon::AtlasDataSet::LGN, "AtlasModel/Dataset/LGN"},
    {AtlasCommon::AtlasDataSet::PAG, "AtlasModel/Dataset/PAG"}
  };

  Model::Model()
  {
    InitializeModel();
  }

  // Should this return by rvalue reference?
  auto Model::GetBestFits(const std::string_view imageName) const -> std::array<AtlasImage::Image, 5>
  {
    std::array<AtlasImage::Image, 5> bestFits = {
        AtlasImage::Image{"1"},
        AtlasImage::Image{"2"},
        AtlasImage::Image{"3"},
        AtlasImage::Image{"4"},
        AtlasImage::Image{"5"}
    };
    return bestFits;
  }

  auto Model::LoadDataSet(const AtlasCommon::AtlasDataSet dataSet) -> void
  {
    std::println("Entered LoadDataSet");
    std::println("Data set attempted to be loaded from map: {}", m_dataSetPaths.at(dataSet));
    std::println("Current path: {}", std::filesystem::current_path().string());
    const std::filesystem::path dataSetPath{std::filesystem::current_path()/=m_dataSetPaths.at(dataSet)};
    std::println("Loading dataset: {}", dataSetPath.string());
    std::ranges::for_each(std::filesystem::directory_iterator{dataSetPath}, [&,this](const auto& entry)
    {
      std::println("Image found: {}", entry.path().string());
      m_images.emplace_back(std::make_unique<AtlasImage::Image>(entry.path().string()));
      std::println("Image loaded: {}", m_images.back()->GetImageName());
    });

    if(m_images.empty())
      AtlasMessenger::Messenger::Instance().SendMessage("No images found in dataset", AtlasCommon::AtlasClasses::AtlasImageViewer);
    else
      AtlasMessenger::Messenger::Instance().SendMessage("Images loaded successfully", AtlasCommon::AtlasClasses::AtlasImageViewer); 
  }

  auto Model::HandleMessage(const char* message) -> void
  {
    auto msg = std::string{message};

    auto commaPos = msg.find(',');
    if(commaPos != std::string::npos)
    {
      auto command = msg.substr(0, commaPos);
      auto argument = msg.substr(commaPos + 1);

      if(command == "GetBestFits")
      {
        auto bestFits = GetBestFits(argument);
        std::ranges::for_each(bestFits, [](const auto& image)
        {
          AtlasMessenger::Messenger::Instance().SendMessage(image.GetImageName().data(), AtlasCommon::AtlasClasses::AtlasImageViewer);
        });
      }
      else if(command == "LoadDataSet")
      {
        std::println("Loading dataset: {}", argument);
        auto dataSet = static_cast<AtlasCommon::AtlasDataSet>(std::stoi(argument));
        LoadDataSet(dataSet);
      }

    }

  }

  auto Model::InitializeModel() -> void
  {
  }
}