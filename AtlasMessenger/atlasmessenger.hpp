#pragma once

#include "AtlasCommon/atlasenums.hpp"
#include <memory>
#include <mutex>


namespace AtlasImageViewer
{
  class ImageViewer;
}

namespace AtlasModel
{
  class Model;
}


/*
* The Atlas Messenger is a singleton class to separate the work from the GUI. Let Qt handle Qt front end updates
* And the model will handle the heavy lifting. This is common in a model/view practice where we don't want to 
* lag the UI.
*
* Sending a message allow direct communication between the classes. 
* ex: AtlasMessenger::Messenger.Instance()->UpdateState(AtlasCommon::AtlasState::Idle, AtlasCommon::AtlasClasses::Model);
*/
namespace AtlasMessenger
{
  class Messenger
  {
    public:
      static auto Instance() -> Messenger&;

      auto UpdateState(const AtlasCommon::AtlasState state, const AtlasCommon::AtlasClasses classID) -> void;
      auto UpdateState(const AtlasCommon::AtlasState state, const AtlasCommon::AtlasClasses classID, const std::string_view imageInformation) -> void;
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