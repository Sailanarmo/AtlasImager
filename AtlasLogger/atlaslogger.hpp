#pragma once

// Umbrella header — include this everywhere.
// Automatically selects the right logger implementation:
//   WASM build  → AtlasLogger::ConsoleLogger (browser console via emscripten_log)
//   Native build → AtlasLogger::FileLogger   (writes to disk)
// Both are aliased as AtlasLogger::Logger so no call sites need to change.

#ifdef __EMSCRIPTEN__
#  include "atlasconsolelogger.hpp"
   namespace AtlasLogger { using Logger = ConsoleLogger; }
#else
#  include "atlasfilelogger.hpp"
   namespace AtlasLogger { using Logger = FileLogger; }
#endif
