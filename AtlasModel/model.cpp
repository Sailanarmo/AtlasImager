#include "model.hpp"
#include "AtlasMessenger/atlasmessenger.hpp"

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/features2d.hpp>
// #include <opencv2/xfeatures2d.hpp>

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

  auto Model::GetQueryDescriptors(const AtlasImage::Image& img) -> AtlasImage::Image
  {
    cv::Mat descriptors;
    cv::Ptr<cv::ORB> detector = cv::ORB::create();
    auto keyPoints = std::vector<cv::KeyPoint>{};
    detector->detectAndCompute(*img.GetImage(), cv::Mat(), keyPoints, descriptors);

    auto atlasDescriptor = AtlasImage::Image{img.GetImageName()};
    atlasDescriptor.CloneData(descriptors);
    std::println("Query Descriptors obtained and Data cloned.");
    return atlasDescriptor;
  }

  auto Model::CalculateMatchScore(const AtlasImage::Image& targetDescriptors, const AtlasImage::Image& modelDescriptors) -> std::pair<std::string,double>
  {
    std::println("Calculating the score.");
    auto brute_forceMatcher = cv::BFMatcher(cv::NORM_HAMMING, true);
    auto matches = std::vector<cv::DMatch>{};

    std::println("Attempting to get matches");
    brute_forceMatcher.match(*targetDescriptors.GetImage(), *modelDescriptors.GetImage(), matches);

    std::println("Caculating the distances");
    auto totalDistance = double{0.0};

    std::ranges::for_each(matches, [&totalDistance](const cv::DMatch& match){
      totalDistance += match.distance;
    });

    return std::make_pair(std::string{modelDescriptors.GetImageName()}, double{totalDistance/matches.size()});
  }

  // Should this return by rvalue reference?
  auto Model::GetBestFits(const std::string_view imageName) -> std::array<std::string, 3>
  {
    auto bestFits = std::array<std::string,3>{};
    auto img = AtlasImage::Image{imageName.data()};
    auto imgDescriptors = GetQueryDescriptors(img);

    auto candidateDescriptors = std::vector<AtlasImage::Image>{};

    std::ranges::transform(m_images,std::back_inserter(candidateDescriptors),
      [this](const AtlasImage::Image& image){ 
        return GetQueryDescriptors(image); 
      }
    );

    auto scores = candidateDescriptors 
    | std::views::transform([this,imgDescriptors](const AtlasImage::Image& descriptor) { 
        return this->CalculateMatchScore(imgDescriptors,descriptor); 
      }) 
    | std::ranges::to<std::vector<std::pair<std::string,double>>>();

    std::ranges::sort(scores, [](const auto& a, const auto& b) { return a.second < b.second; });

    bestFits[0] = std::string{scores[0].first + ":" + std::to_string(scores[0].second)};
    bestFits[1] = std::string{scores[1].first + ":" + std::to_string(scores[1].second)};
    bestFits[2] = std::string{scores[2].first + ":" + std::to_string(scores[2].second)};

    return bestFits;
  }

  auto Model::LoadDataSet(const AtlasCommon::AtlasDataSet dataSet) -> void
  {
    if(m_images.size() > 0)
      m_images.clear();

    const std::filesystem::path dataSetPath{std::filesystem::current_path()/=m_dataSetPaths.at(dataSet)};
    std::ranges::for_each(std::filesystem::directory_iterator{dataSetPath}, [&,this](const auto& entry)
    {
      m_images.emplace_back(AtlasImage::Image(entry.path().string()));
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
        std::println("GetBestFits called");
        auto bestFits = GetBestFits(argument);
        std::ranges::for_each(bestFits, [](const auto& image)
        {
          auto msg = std::string{"AddImage," + image};
          AtlasMessenger::Messenger::Instance().SendMessage(msg.c_str(), AtlasCommon::AtlasClasses::AtlasImageViewer);
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