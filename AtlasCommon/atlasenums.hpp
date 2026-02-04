#pragma once

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
    LoadingLGNModel,
    LoadingPAGModel,
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

  static const std::unordered_map<AtlasDataSet, const char*> AtlasDataSetNames
  {
    {AtlasDataSet::LGN, "LGN"},
    {AtlasDataSet::PAG, "PAG"}
  };

  using AtlasState = std::variant<AtlasCommon::AtlasModelState, AtlasCommon::AtlasImageViewerState>;
  // One Global State Variable
  static AtlasState CurrentAtlasState;
}
