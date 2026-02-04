#include "modelworker.hpp"
#include "AtlasLogger/atlaslogger.hpp"

#include <filesystem>
#include <ranges>

namespace AtlasModel
{
  static AtlasLogger::Logger m_workerLogger{std::filesystem::current_path().string() + "/Logs/ModelWorker.log", "AtlasModel::ModelWorker"};

  ModelWorker::ModelWorker(QObject* parent) : QObject(parent)
  {
  }

  void ModelWorker::LoadImages(const std::filesystem::path& datasetPath)
  {
    auto images = std::vector<AtlasImage::Image>{};

    if(!std::filesystem::exists(datasetPath))
    {
      m_workerLogger.Log(AtlasLogger::LogLevel::Error, "Dataset path not found: {}", datasetPath.string());
      emit LoadingFailed(QString::fromStdString("Dataset path not found: " + datasetPath.string()));
      return;
    }

    m_workerLogger.Log(AtlasLogger::LogLevel::Info, "Loading images from dataset path: {}", datasetPath.string());
    
    auto start = std::filesystem::directory_iterator{datasetPath};
    auto end = std::filesystem::directory_iterator{};

    const auto count = std::distance(start, end);
    m_workerLogger.Log(AtlasLogger::LogLevel::Info, "Found {} images in dataset: {}", count, datasetPath.string());
    
    m_workerLogger.Log(AtlasLogger::LogLevel::Info, "Emitting ProgressMaximum signal with value: {}", count);
    emit ProgressMaximum(count);

    for(const auto& [index, entry] : std::views::enumerate(std::filesystem::directory_iterator{datasetPath}))
    {
      images.emplace_back(AtlasImage::Image(entry.path().string()));
      m_workerLogger.Log(AtlasLogger::LogLevel::Info, "Loading image {}: {}", index + 1, entry.path().filename().string());
      emit ProgressUpdate(index + 1);
    }

    if(images.empty())
    {
      m_workerLogger.Log(AtlasLogger::LogLevel::Warning, "No images found in dataset path: {}", datasetPath.string());
      emit LoadingFailed(QString::fromStdString("No images found in dataset path: " + datasetPath.string()));
      return;
    }

    m_workerLogger.Log(AtlasLogger::LogLevel::Info, "Successfully loaded {} images", images.size());
    m_workerLogger.Log(AtlasLogger::LogLevel::Info, "Emitting LoadingComplete signal");
    emit LoadingComplete(std::move(images));
  }
}
