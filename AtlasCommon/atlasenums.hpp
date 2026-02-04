#pragma once

#include <string>
#include <cstdint>
#include <variant>
#include <unordered_map>

namespace AtlasCommon
{
  enum class AtlasDataSet : std::uint8_t
  {
    LGN = 0,
    PAG
  };

  enum class AtlasModelState : std::uint8_t
  {
    Idle = 0,
    LoadLGNModel,
    LoadPAGModel,
    ModelPAGLoaded,
    FindingBestFits,
    SendingBestFits,
    LoadingUserImage,
    UserImageLoaded,
    RotatingImage,
    ImageReset,
    NextImage,
    PreviousImage
  };

  enum class AtlasImageViewerState : std::uint8_t
  {
    Idle = 0,
    ConstructPopup,
    SetMaximumProgressBarValue,
    UpdateProgressBarValue,
    DisplayPopup,
    DestroyPopup,
    AddImage,
    LoadImage,
    NextImage,
    PreviousImage,
    SliderUpdated,
    RotateImage
  };

  enum class AtlasClasses : std::uint8_t
  {
    Idle = 0,
    AtlasModel,
    AtlasImageViewer
  };

  static const std::unordered_map<std::string, AtlasDataSet> AtlasDataSetNames
  {
    {"LGN", AtlasDataSet::LGN},
    {"PAG", AtlasDataSet::PAG}
  };

  static auto DataSetToString(const AtlasDataSet dataSet) -> std::string
  {
    switch(dataSet)
    {
      case AtlasDataSet::LGN:
        return "LGN";
      case AtlasDataSet::PAG:
        return "PAG";
      default:
        return "Unknown";
    }
  }

  using AtlasState = std::variant<AtlasCommon::AtlasModelState, AtlasCommon::AtlasImageViewerState>;
  // One Global State Variable
  static AtlasState CurrentAtlasState;
}
