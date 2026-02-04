#include "model.hpp"
#include "AtlasMessenger/atlasmessenger.hpp"
#include "AtlasLogger/atlaslogger.hpp"

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/features2d.hpp>

#include <thread>
#include <ranges>
#include <algorithm>
#include <filesystem>

#include <print>

#include <expected>

namespace AtlasModel
{
  const std::unordered_map<AtlasCommon::AtlasDataSet, std::string> Model::m_dataSetPaths =
  {
    {AtlasCommon::AtlasDataSet::LGN, "/Dataset/LGN"},
    {AtlasCommon::AtlasDataSet::PAG, "/Dataset/PAG"}
  };

  static AtlasLogger::Logger m_logger{std::filesystem::current_path().string() + "/Logs/Model.log", "AtlasModel::Model"};

  enum class LoadDataSetResult
  {
    Success,
    PathNotFound,
    NoImagesFound
  };

  auto GetImages(const std::filesystem::path& datasetPath) -> std::expected<std::vector<AtlasImage::Image>, LoadDataSetResult>
  {
    auto images = std::vector<AtlasImage::Image>{};

    if(!std::filesystem::exists(datasetPath))
    {
      // Should not happen if paths are correct
      m_logger.Log(AtlasLogger::LogLevel::Error, "Dataset path not found: {}", datasetPath.string());
      return std::unexpected(LoadDataSetResult::PathNotFound);
    }

    for(const auto& entry : std::filesystem::directory_iterator{datasetPath})
    {
      images.emplace_back(AtlasImage::Image(entry.path().string()));
    }

    if(images.empty())
    {
      m_logger.Log(AtlasLogger::LogLevel::Warning, "No images found in dataset path: {}", datasetPath.string());
      return std::unexpected(LoadDataSetResult::NoImagesFound);
    }
    return images;
  }

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
  auto Model::GetBestFits(const std::string_view imageName) -> std::vector<std::string> {
    auto bestFits = std::vector<std::string>{};
    double num = 0;
    for(auto &image : m_images) {
      auto name = image.GetImageName();
      m_logger.Log(AtlasLogger::LogLevel::Info, "Best fit image name: {}", name);
      bestFits.push_back(std::string(name) + ":" + std::to_string(num));
      num += 1;
    }
    return bestFits;
  }
//  auto Model::GetBestFits(const std::string_view imageName) -> std::array<std::string, 3>
//  {
//    auto bestFits = std::array<std::string,3>{};
//    auto img = AtlasImage::Image{imageName.data()};
//    auto imgDescriptors = GetQueryDescriptors(img);
//
//    auto candidateDescriptors = std::vector<AtlasImage::Image>{};
//
//    std::ranges::transform(m_images,std::back_inserter(candidateDescriptors),
//      [this](const AtlasImage::Image& image){
//        return GetQueryDescriptors(image);
//      }
//    );
//
//    auto scores = candidateDescriptors
//    | std::views::transform([this,imgDescriptors](const AtlasImage::Image& descriptor) {
//        return this->CalculateMatchScore(imgDescriptors,descriptor);
//      })
//    | std::ranges::to<std::vector<std::pair<std::string,double>>>();
//
//    std::ranges::sort(scores, [](const auto& a, const auto& b) { return a.second < b.second; });
//
//    bestFits[0] = std::string{scores[0].first + ":" + std::to_string(scores[0].second)};
//    bestFits[1] = std::string{scores[1].first + ":" + std::to_string(scores[1].second)};
//    bestFits[2] = std::string{scores[2].first + ":" + std::to_string(scores[2].second)};
//    std::println("{}", bestFits);
//    return bestFits;
//  }

  auto Model::LoadDataSet(const AtlasCommon::AtlasDataSet dataSet) -> void
  {
    if(m_images.size() > 0)
      m_images.clear();

    const auto currentPath = std::filesystem::current_path().string();
    const auto modelPath = m_dataSetPaths.at(dataSet);

    m_logger.Log(AtlasLogger::LogLevel::Info, "Loading dataset from path: {}", currentPath + modelPath);

    const auto dataSetPath = std::filesystem::path{currentPath + modelPath};

    m_images = GetImages(dataSetPath).value_or(std::vector<AtlasImage::Image>{});

    if(m_images.empty())
    {
      m_logger.Log(AtlasLogger::LogLevel::Error, "No images found in dataset at path: {}", dataSetPath.string());
      AtlasMessenger::Messenger::Instance().UpdateState(AtlasCommon::AtlasImageViewerState::Idle, AtlasCommon::AtlasClasses::AtlasImageViewer);
    }
    else
    {
      m_logger.Log(AtlasLogger::LogLevel::Info, "Successfully loaded {} images from dataset", m_images.size());
      AtlasMessenger::Messenger::Instance().UpdateState(AtlasCommon::AtlasImageViewerState::Idle, AtlasCommon::AtlasClasses::AtlasImageViewer); 
    }
  }

  auto Model::ProcessBestFits(const std::string_view imageName) -> void
  {
    auto bestFits = this->GetBestFits(imageName);
    std::sort(bestFits.begin(), bestFits.end());
    std::ranges::for_each(bestFits, [](const auto& image)
    {
      AtlasMessenger::Messenger::Instance().UpdateState(AtlasCommon::AtlasImageViewerState::AddImage, AtlasCommon::AtlasClasses::AtlasImageViewer, image);
    });

    AtlasMessenger::Messenger::Instance().UpdateState(AtlasCommon::AtlasImageViewerState::Idle, AtlasCommon::AtlasClasses::AtlasImageViewer);
  }

  auto Model::HandleStateUpdate(const AtlasCommon::AtlasModelState state, const std::string_view userImage) -> void
  {
    switch(state)
    {
      case AtlasCommon::AtlasModelState::Idle:
        // Do nothing
        return;
      case AtlasCommon::AtlasModelState::LoadLGNModel:
        m_logger.Log(AtlasLogger::LogLevel::Info, "State Update: Loading LGN Model Data...");
        this->LoadDataSet(AtlasCommon::AtlasDataSet::LGN);
        break;
      case AtlasCommon::AtlasModelState::LoadPAGModel:
        m_logger.Log(AtlasLogger::LogLevel::Info, "State Update: Loading PAG Model Data...");
        this->LoadDataSet(AtlasCommon::AtlasDataSet::PAG);
        break;
      case AtlasCommon::AtlasModelState::FindingBestFits:
        m_logger.Log(AtlasLogger::LogLevel::Info, "State Update: Finding Best Fits...");
        this->ProcessBestFits(userImage);
        break;
      default:
        break;
    }
  }

  auto Model::InitializeModel() -> void
  {
    m_logger.Log(AtlasLogger::LogLevel::Info, "Model initialized.");
    AtlasCommon::CurrentAtlasState = AtlasCommon::AtlasModelState::Idle;
  }

}