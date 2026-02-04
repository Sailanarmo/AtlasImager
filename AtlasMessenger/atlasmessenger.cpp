#include "atlasmessenger.hpp"
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
  auto Messenger::UpdateState(const AtlasCommon::AtlasState state, const AtlasCommon::AtlasClasses classID) -> void
  {
    // Keep it simple!! 
    switch(classID)
    {
      case AtlasCommon::AtlasClasses::Idle:
        // Do nothing
        break;
      case AtlasCommon::AtlasClasses::AtlasModel:
        m_model->HandleStateUpdate(std::get<AtlasCommon::AtlasModelState>(state));
        break;
      case AtlasCommon::AtlasClasses::AtlasImageViewer:
        m_imageViewer->HandleStateUpdate(std::get<AtlasCommon::AtlasImageViewerState>(state));
        break;
      default:
        break;
    }
  }

  auto Messenger::UpdateState(const AtlasCommon::AtlasState state, const AtlasCommon::AtlasClasses classID, const std::string_view imageInformation) -> void
  {
    // Keep it simple!! 
    switch(classID)
    {
      case AtlasCommon::AtlasClasses::Idle:
        // Do nothing
        break;
      case AtlasCommon::AtlasClasses::AtlasModel:
        m_model->HandleStateUpdate(std::get<AtlasCommon::AtlasModelState>(state), imageInformation);
        break;
      case AtlasCommon::AtlasClasses::AtlasImageViewer:
        m_imageViewer->HandleStateUpdate(std::get<AtlasCommon::AtlasImageViewerState>(state), imageInformation);
        break;
      default:
        break;
    }
  }

  auto Messenger::UpdateState(const AtlasCommon::AtlasState state, const AtlasCommon::AtlasClasses classID, const int value) -> void
  {
    // Keep it simple!! 
    switch(classID)
    {
      case AtlasCommon::AtlasClasses::Idle:
        // Do nothing
        break;
      case AtlasCommon::AtlasClasses::AtlasModel:
        // No int handling for model yet
        break;
      case AtlasCommon::AtlasClasses::AtlasImageViewer:
        m_imageViewer->HandleStateUpdate(std::get<AtlasCommon::AtlasImageViewerState>(state), value);
        break;
      default:
        break;
    }
  }

  auto Messenger::UpdateState(const AtlasCommon::AtlasState state, const AtlasCommon::AtlasClasses classID, const AtlasCommon::AtlasDataSet dataSet) -> void
  {
    // Keep it simple!! 
    switch(classID)
    {
      case AtlasCommon::AtlasClasses::Idle:
        // Do nothing
        break;
      case AtlasCommon::AtlasClasses::AtlasModel:
        // No viewer state handling for model yet
        break;
      case AtlasCommon::AtlasClasses::AtlasImageViewer:
        m_imageViewer->HandleStateUpdate(std::get<AtlasCommon::AtlasImageViewerState>(state), dataSet);
        break;
      default:
        break;
    }
  }
}