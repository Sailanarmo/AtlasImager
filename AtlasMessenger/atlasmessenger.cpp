#include "atlasmessenger.hpp"
#include "AtlasCommon/atlasenums.hpp"
#include "AtlasImageViewer/atlasimageviewer.hpp"
#include "AtlasModel/model.hpp"

namespace AtlasMessenger
{
  auto Messenger::Instance() -> Messenger&
  {
    static Messenger instance;
    return instance;
  }

  auto Messenger::SetImageViewer(AtlasImageViewer::ImageViewer* imageViewer) -> void
  {
    m_imageViewer = imageViewer;
  }

  auto Messenger::SetModel(AtlasModel::Model* model) -> void
  {
    m_model = model;
  }

  // This function handles the message based on the classID it recieves.
  auto Messenger::SendMessage(const char* message, const AtlasCommon::AtlasClasses classID) -> void
  {
    // Keep it simple!! 
    switch(classID)
    {
      case AtlasCommon::AtlasClasses::AtlasModel:
        m_model->HandleMessage(message);
        break;
      case AtlasCommon::AtlasClasses::AtlasImageViewer:
        m_imageViewer->HandleMessage(message);
        break;
    }
  }
}