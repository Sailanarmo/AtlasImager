#include "model.hpp"

#include "AtlasImage/image.hpp"
#include "AtlasMessenger/atlasmessenger.hpp"
#include "AtlasLogger/atlaslogger.hpp"

#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>

#include <mutex>
#include <atomic>
#include <thread>
#include <ranges>
#include <algorithm>
#include <filesystem>


//#include <QCoreApplication>

namespace AtlasModel
{
  const std::unordered_map<AtlasCommon::AtlasDataSet, std::string> Model::m_dataSetPaths =
  {
    {AtlasCommon::AtlasDataSet::LGN, "/Dataset/LGN"},
    {AtlasCommon::AtlasDataSet::PAG, "/Dataset/PAG"}
  };

  static AtlasLogger::Logger m_logger{
#ifdef __EMSCRIPTEN__
    "",
#else
    QStandardPaths::standardLocations(QStandardPaths::AppConfigLocation).at(1).toStdString() +
    "/Atlas-Imager/Logs/" + AtlasLogger::GetCurrentDateString() + "/Model.log",
#endif
    "AtlasModel::Model"
  };


  auto GetImages(const std::filesystem::path& datasetPath) -> std::expected<std::vector<AtlasImage::Image>, Model::LoadDataSetResult>
  {
    auto images = std::vector<AtlasImage::Image>{};

    if(!std::filesystem::exists(datasetPath))
    {
      // Should not happen if paths are correct
      m_logger.Log(AtlasLogger::LogLevel::Error, "Dataset path not found: {}", datasetPath.string());
      return std::unexpected(Model::LoadDataSetResult::PathNotFound);
    }
    else
    {
      m_logger.Log(AtlasLogger::LogLevel::Info, "Loading images from dataset path: {}", datasetPath.string());
      auto start = std::filesystem::directory_iterator{datasetPath};
      auto end = std::filesystem::directory_iterator{};

      const auto count = std::distance(start, end);
      m_logger.Log(AtlasLogger::LogLevel::Info, "Found {} images in dataset: {}", count, datasetPath.string());
      AtlasMessenger::Messenger::Instance().UpdateState(AtlasCommon::AtlasImageViewerState::SetMaximumProgressBarValue, AtlasCommon::AtlasClasses::AtlasImageViewer, count);
      AtlasMessenger::Messenger::Instance().UpdateState(AtlasCommon::AtlasImageViewerState::DisplayPopup, AtlasCommon::AtlasClasses::AtlasImageViewer);
      
      // Give the GUI event loop a chance to display the popup before we start the blocking image load
      //QCoreApplication::processEvents();
    }
    
#ifdef __EMSCRIPTEN__
    auto index = std::ptrdiff_t{0};
    for(const auto& entry : std::filesystem::directory_iterator{datasetPath})
    {
      images.emplace_back(AtlasImage::Image(entry.path().string()));
      AtlasMessenger::Messenger::Instance().UpdateState(AtlasCommon::AtlasImageViewerState::UpdateProgressBarValue, AtlasCommon::AtlasClasses::AtlasImageViewer, index + 1);
      ++index;
    }
#else
    for(const auto& [index, entry] : std::views::enumerate(std::filesystem::directory_iterator{datasetPath}) )
    {
      images.emplace_back(AtlasImage::Image(entry.path().string()));
      AtlasMessenger::Messenger::Instance().UpdateState(AtlasCommon::AtlasImageViewerState::UpdateProgressBarValue, AtlasCommon::AtlasClasses::AtlasImageViewer, index + 1);
    }
#endif

    if(images.empty())
    {
      m_logger.Log(AtlasLogger::LogLevel::Warning, "No images found in dataset path: {}", datasetPath.string());
      return std::unexpected(Model::LoadDataSetResult::NoImagesFound);
    }
    return images;
  }

  Model::Model()
  {
    InitializeModel();
  }

  Model::~Model()
  {
    m_logger.Log(AtlasLogger::LogLevel::Info, "Model Destructor Called. Clearing loaded images.");
    m_images.clear();
  }

  auto Model::LoadDataSet(const AtlasCommon::AtlasDataSet dataSet) -> void
  {
    if(m_images.size() > 0)
      m_images.clear();

    const auto currentPath = std::filesystem::current_path().string();
    const auto modelPath = m_dataSetPaths.at(dataSet);

    m_logger.Log(AtlasLogger::LogLevel::Info, "Loading dataset from path: {}", currentPath + modelPath);

    const auto dataSetPath = std::filesystem::path{currentPath + modelPath};

    const auto mainLabelText = "Loading " + AtlasCommon::DataSetToString(dataSet) + " Model Images...";
    const auto progressBarTextFormat = "%v of %m " + AtlasCommon::DataSetToString(dataSet) + " Images loaded %p%";

    AtlasMessenger::Messenger::Instance().UpdateState(AtlasCommon::AtlasImageViewerState::ConstructPopup, AtlasCommon::AtlasClasses::AtlasImageViewer, mainLabelText, progressBarTextFormat);

    m_images = GetImages(dataSetPath).value_or(std::vector<AtlasImage::Image>{});

    if(m_images.empty())
    {
      m_logger.Log(AtlasLogger::LogLevel::Error, "No images found in dataset at path: {}", dataSetPath.string());
      AtlasMessenger::Messenger::Instance().UpdateState(AtlasCommon::AtlasImageViewerState::DestroyPopup, AtlasCommon::AtlasClasses::AtlasImageViewer); 
      AtlasMessenger::Messenger::Instance().UpdateState(AtlasCommon::AtlasImageViewerState::Idle, AtlasCommon::AtlasClasses::AtlasImageViewer);
    }
    else
    {
      m_logger.Log(AtlasLogger::LogLevel::Info, "Successfully loaded {} images from dataset", m_images.size());
      AtlasMessenger::Messenger::Instance().UpdateState(AtlasCommon::AtlasImageViewerState::DestroyPopup, AtlasCommon::AtlasClasses::AtlasImageViewer); 
      AtlasMessenger::Messenger::Instance().UpdateState(AtlasCommon::AtlasImageViewerState::Idle, AtlasCommon::AtlasClasses::AtlasImageViewer);
    }
  }

  auto Model::GetAllModelImagePaths() -> std::expected<std::vector<std::string>, LoadDataSetResult>
  {
    auto imagePaths = std::vector<std::string>{};
    for(const auto& image : m_images)
    {
      imagePaths.emplace_back(std::string{image.GetImageName()});
    }

    if(imagePaths.empty())
    {
      m_logger.Log(AtlasLogger::LogLevel::Error, "No model images loaded to retrieve paths from.");
      return std::unexpected(LoadDataSetResult::NoImagesFound);
    }

    return imagePaths;
  }

  auto Model::LoadAllDataSetImages(const AtlasCommon::AtlasDataSet dataSet) -> void
  {
    if(m_images.empty())
    {
      m_logger.Log(AtlasLogger::LogLevel::Error, "No model images loaded to retrieve paths from.");
      return;
    }

    const auto mainLabelText = "Rendering All " + AtlasCommon::DataSetToString(dataSet) + " Model Images...";
    const auto progressBarTextFormat = "%v of %m " + AtlasCommon::DataSetToString(dataSet) + " Images rendered %p%";

    AtlasMessenger::Messenger::Instance().UpdateState(AtlasCommon::AtlasImageViewerState::ConstructPopup, AtlasCommon::AtlasClasses::AtlasImageViewer, mainLabelText, progressBarTextFormat);
    const auto imagePaths = this->GetAllModelImagePaths().value_or(std::vector<std::string>{});

    if(imagePaths.empty())
    {
      m_logger.Log(AtlasLogger::LogLevel::Error, "No model image paths retrieved.");
      AtlasMessenger::Messenger::Instance().UpdateState(AtlasCommon::AtlasImageViewerState::DestroyPopup, AtlasCommon::AtlasClasses::AtlasImageViewer); 
      return;
    }
   
    const auto count = static_cast<int>(imagePaths.size());
    AtlasMessenger::Messenger::Instance().UpdateState(AtlasCommon::AtlasImageViewerState::SetMaximumProgressBarValue, AtlasCommon::AtlasClasses::AtlasImageViewer, count);
    AtlasMessenger::Messenger::Instance().UpdateState(AtlasCommon::AtlasImageViewerState::DisplayPopup, AtlasCommon::AtlasClasses::AtlasImageViewer);
      
    // Give the GUI event loop a chance to display the popup before we start the blocking image load
    //QCoreApplication::processEvents();

#ifdef __EMSCRIPTEN__
    auto index = std::ptrdiff_t{0};
    for(const auto& path : imagePaths)
    {
      m_logger.Log(AtlasLogger::LogLevel::Info, "Sending model image to ImageViewer: {}", path);
      AtlasMessenger::Messenger::Instance().UpdateState(AtlasCommon::AtlasImageViewerState::AddImage, AtlasCommon::AtlasClasses::AtlasImageViewer, path);
      AtlasMessenger::Messenger::Instance().UpdateState(AtlasCommon::AtlasImageViewerState::UpdateProgressBarValue, AtlasCommon::AtlasClasses::AtlasImageViewer, index + 1);
      ++index;
    }
#else
    for(const auto& [index, path] : std::views::enumerate(imagePaths))
    {
      m_logger.Log(AtlasLogger::LogLevel::Info, "Sending model image to ImageViewer: {}", path);
      AtlasMessenger::Messenger::Instance().UpdateState(AtlasCommon::AtlasImageViewerState::AddImage, AtlasCommon::AtlasClasses::AtlasImageViewer, path);
      AtlasMessenger::Messenger::Instance().UpdateState(AtlasCommon::AtlasImageViewerState::UpdateProgressBarValue, AtlasCommon::AtlasClasses::AtlasImageViewer, index + 1);
    }
#endif

    AtlasMessenger::Messenger::Instance().UpdateState(AtlasCommon::AtlasImageViewerState::DestroyPopup, AtlasCommon::AtlasClasses::AtlasImageViewer); 
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
        m_loadedDataSet = AtlasCommon::AtlasDataSet::LGN;
        std::jthread(
          [this]()
          {
            this->LoadDataSet(m_loadedDataSet);
          }
        ).detach();
        break;
      case AtlasCommon::AtlasModelState::LoadPAGModel:
        m_logger.Log(AtlasLogger::LogLevel::Info, "State Update: Loading PAG Model Data...");
        m_loadedDataSet = AtlasCommon::AtlasDataSet::PAG;
        std::jthread(
          [this]()
          {
            this->LoadDataSet(m_loadedDataSet);
          }
        ).detach();
        break;
      case AtlasCommon::AtlasModelState::LoadAllModelImages:
        m_logger.Log(AtlasLogger::LogLevel::Info, "State Update: Loading All Model Images...");
        this->LoadAllDataSetImages(m_loadedDataSet);
        AtlasMessenger::Messenger::Instance().UpdateState(AtlasCommon::AtlasImageViewerState::UpdateRenderer, AtlasCommon::AtlasClasses::AtlasImageViewer);
        AtlasMessenger::Messenger::Instance().UpdateState(AtlasCommon::AtlasImageViewerState::Idle, AtlasCommon::AtlasClasses::AtlasImageViewer);
        break;
      case AtlasCommon::AtlasModelState::FindingBestFits:
        m_logger.Log(AtlasLogger::LogLevel::Info, "State Update: Finding Best Fits...");
        //this->ProcessBestFits(userImage);
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

  /*
  auto Model::GetQueryDescriptors(const AtlasImage::Image& img) -> AtlasImage::Image
  {
    cv::Mat descriptors;
    cv::Ptr<cv::ORB> detector = cv::ORB::create();
    auto keyPoints = std::vector<cv::KeyPoint>{};
    detector->detectAndCompute(*img.GetImage(), cv::Mat(), keyPoints, descriptors);

    auto atlasDescriptor = AtlasImage::Image{img.GetImageName()};
    atlasDescriptor.CloneData(descriptors);
    m_logger.Log(AtlasLogger::LogLevel::Info, "Obtained {} keypoints for image: {}", keyPoints.size(), img.GetImageName());
    return atlasDescriptor;
  }

  auto Model::CalculateMatchScore(const AtlasImage::Image& targetDescriptors, const AtlasImage::Image& modelDescriptors) -> std::pair<std::string,double>
  {
    m_logger.Log(AtlasLogger::LogLevel::Info, "Calculating match score between target: {} and model: {}", targetDescriptors.GetImageName(), modelDescriptors.GetImageName());
    auto brute_forceMatcher = cv::BFMatcher(cv::NORM_HAMMING, true);
    auto matches = std::vector<cv::DMatch>{};

    m_logger.Log(AtlasLogger::LogLevel::Info, "Matching descriptors...");
    brute_forceMatcher.match(*targetDescriptors.GetImage(), *modelDescriptors.GetImage(), matches);

    m_logger.Log(AtlasLogger::LogLevel::Info, "Calculating the distances");
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
    std::println("{}", bestFits);
    return bestFits;
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
  */

}