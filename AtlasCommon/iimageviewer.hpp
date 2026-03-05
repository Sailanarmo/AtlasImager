#pragma once

#include "atlasenums.hpp"
#include <string_view>

namespace AtlasCommon
{
  /*
  * IImageViewer is a pure interface that decouples AtlasMessenger from any
  * specific renderer implementation. Both the Qt desktop build (AtlasImageViewer)
  * and the WebAssembly build (AtlasImageViewerWeb) implement this interface, so
  * AtlasMessenger never needs to know which backend is active.
  */
  class IImageViewer
  {
  public:
    virtual ~IImageViewer() = default;

    virtual auto HandleStateUpdate(AtlasImageViewerState state,
                                   std::string_view imageInformation = "") -> void = 0;

    virtual auto HandleStateUpdate(AtlasImageViewerState state,
                                   std::string_view mainLabelText,
                                   std::string_view progressBarTextFormat) -> void = 0;

    virtual auto HandleStateUpdate(AtlasImageViewerState state,
                                   int value) -> void = 0;
  };
}
