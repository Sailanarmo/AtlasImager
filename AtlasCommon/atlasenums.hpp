#pragma once

#include <unordered_map>

namespace AtlasCommon
{
  enum class AtlasDataSet : int
  {
    LGN = 0,
    PAG
  };

  enum class AtlasClasses : int
  {
    AtlasModel = 0,
    AtlasImageViewer
  };

  static const std::unordered_map<AtlasDataSet, const char*> AtlasDataSetNames
  {
    {AtlasDataSet::LGN, "LGN"},
    {AtlasDataSet::PAG, "PAG"}
  };
}