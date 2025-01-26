#pragma once

#include <memory>
#include <mutex>

namespace AtlasCommon
{
  enum class AtlasClasses;
}

namespace AtlasImageViewer
{
  class ImageViewer;
}

namespace AtlasModel
{
  class Model;
}

namespace AtlasMessenger
{
  class Messenger
  {
    public:
      static auto Instance() -> Messenger&;

      auto SendMessage(const char* message, const AtlasCommon::AtlasClasses classID) -> void;
      auto SetImageViewer(AtlasImageViewer::ImageViewer* imageViewer) -> void;
      auto SetModel(AtlasModel::Model* model) -> void;

    private:
      AtlasImageViewer::ImageViewer* m_imageViewer{nullptr};
      AtlasModel::Model* m_model{nullptr};

      Messenger() = default;
      ~Messenger() = default;

      Messenger(const Messenger&) = delete;
      Messenger& operator=(const Messenger&) = delete;
  };
}