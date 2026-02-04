#pragma once

#include "AtlasCommon/atlasenums.hpp"
#include "AtlasImage/image.hpp"

#include <QObject>
#include <filesystem>
#include <vector>

namespace AtlasModel
{
  class ModelWorker : public QObject
  {
    Q_OBJECT

  public:
    explicit ModelWorker(QObject* parent = nullptr);
    ~ModelWorker() override = default;

  public slots:
    void LoadImages(const std::filesystem::path& datasetPath);

  signals:
    void ProgressMaximum(int max);
    void ProgressUpdate(int current);
    void LoadingComplete(std::vector<AtlasImage::Image> images);
    void LoadingFailed(const QString& errorMessage);
  };
}
