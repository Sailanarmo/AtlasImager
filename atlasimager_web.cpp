#include "AtlasImageViewerWeb/atlasimageviewerweb.hpp"
#include "AtlasModel/model.hpp"
#include "AtlasMessenger/atlasmessenger.hpp"

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/html5.h>
#include <emscripten/val.h>

#include <memory>
#include <string>

// ---------------------------------------------------------------------------
// Module-level singletons
// ---------------------------------------------------------------------------
namespace
{
  AtlasImageViewerWeb::ImageViewer g_viewer;
  std::unique_ptr<AtlasModel::Model> g_model;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

// Called once from atlas.html <script> after the WASM module has loaded.
// canvasId should match the id attribute of the <canvas> element.
auto initRenderer(int canvasWidth, int canvasHeight) -> void
{
  // Wire the viewer into the messenger so AtlasModel can dispatch state updates
  // to it exactly the same way the Qt desktop build does.
  AtlasMessenger::Messenger::Instance().SetImageViewer(&g_viewer);

  g_viewer.InitializeGL(canvasWidth, canvasHeight);
}

// Called from JS requestAnimationFrame loop.
auto paint() -> void
{
  g_viewer.Paint();
}

auto resizeRenderer(int width, int height) -> void
{
  g_viewer.Resize(width, height);
}

// ---------------------------------------------------------------------------
// Model
// ---------------------------------------------------------------------------

// Initialise and start the matching model. In the WASM build there is no
// detached thread — Emscripten's asyncify or Wasm workers would be used for
// true background work, but for now the model runs synchronously on demand.
auto initModel() -> void
{
  g_model = std::make_unique<AtlasModel::Model>();
  AtlasMessenger::Messenger::Instance().SetModel(g_model.get());
}

// Trigger the model to find the best atlas matches for the currently loaded
// user image. Results are dispatched back through the messenger → g_viewer.
auto runMatching(const std::string& datasetName) -> void
{
  if(!g_model) return;
  AtlasMessenger::Messenger::Instance().UpdateState(
    AtlasCommon::AtlasState{ AtlasCommon::AtlasModelState::LoadingUserImage },
    AtlasCommon::AtlasClasses::AtlasModel,
    datasetName
  );
}

// ---------------------------------------------------------------------------
// Image loading
// ---------------------------------------------------------------------------

// Load the base (user XRay) image by virtual filesystem path.
// In the browser, files must be pre-loaded into the Emscripten virtual FS
// (e.g. via Module.FS_createDataFile or Module.FS.writeFile from JS).
auto loadMainImage(const std::string& path) -> void
{
  AtlasMessenger::Messenger::Instance().UpdateState(
    AtlasCommon::AtlasState{ AtlasCommon::AtlasImageViewerState::LoadImage },
    AtlasCommon::AtlasClasses::AtlasImageViewer,
    path
  );
}

// ---------------------------------------------------------------------------
// Pixel-based image loading (preferred WASM path — no cv::imread needed)
// ---------------------------------------------------------------------------

// Transfer a JS Uint8Array into WASM linear memory, returning it as a vector.
static auto JsArrayToVec(const emscripten::val& jsArray) -> std::vector<uint8_t>
{
  const auto len = jsArray["length"].as<unsigned>();
  std::vector<uint8_t> buf(len);
  // Create a Uint8Array view over our freshly allocated WASM buffer and
  // call .set() to copy the JS-side bytes in a single JS call (fast path).
  emscripten::val::global("Uint8Array")
    .new_(emscripten::val::module_property("HEAPU8")["buffer"],
          reinterpret_cast<uintptr_t>(buf.data()), len)
    .call<void>("set", jsArray);
  return buf;
}

// Called from TypeScript with decoded TIFF pixel data (Uint8Array).
// channels: 1 = grayscale, 3 = RGB, 4 = RGBA
auto loadMainImageFromPixels(emscripten::val pixelData,
                             int width, int height, int channels) -> void
{
  auto buf = JsArrayToVec(pixelData);
  g_viewer.LoadMainImageFromPixels(buf.data(), width, height, channels);
}

auto addOverlayImageFromPixels(emscripten::val pixelData,
                               int width, int height, int channels) -> void
{
  auto buf = JsArrayToVec(pixelData);
  g_viewer.AddOverlayImageFromPixels(buf.data(), width, height, channels);
}

// ---------------------------------------------------------------------------
// Viewer controls — called directly from TypeScript UI
// ---------------------------------------------------------------------------

auto setOpacity(double value) -> void   { g_viewer.SetOpacity(value); }
auto setRotation(double degrees) -> void { g_viewer.SetRotation(degrees); }
auto setScale(double scale) -> void     { g_viewer.SetScale(scale); }

auto nextImage() -> void   { g_viewer.NextImage(); }
auto prevImage() -> void   { g_viewer.PrevImage(); }
auto resetImage() -> void  { g_viewer.ResetImage(); }
auto saveImage() -> void   { g_viewer.SaveImage(); }

auto moveImageLeft()    -> void { g_viewer.MoveImageLeft(); }
auto moveImageRight()   -> void { g_viewer.MoveImageRight(); }
auto moveImageUp()      -> void { g_viewer.MoveImageUp(); }
auto moveImageDown()    -> void { g_viewer.MoveImageDown(); }
auto moveOverlayLeft()  -> void { g_viewer.MoveOverlayLeft(); }
auto moveOverlayRight() -> void { g_viewer.MoveOverlayRight(); }
auto moveOverlayUp()    -> void { g_viewer.MoveOverlayUp(); }
auto moveOverlayDown()  -> void { g_viewer.MoveOverlayDown(); }

// ---------------------------------------------------------------------------
// Emscripten bindings — these become Module.functionName() in TypeScript
// ---------------------------------------------------------------------------
EMSCRIPTEN_BINDINGS(atlas_imager)
{
  emscripten::function("initRenderer",    &initRenderer);
  emscripten::function("paint",           &paint);
  emscripten::function("resizeRenderer",  &resizeRenderer);

  emscripten::function("initModel",       &initModel);
  emscripten::function("runMatching",     &runMatching);

  emscripten::function("loadMainImage",            &loadMainImage);
  emscripten::function("loadMainImageFromPixels",   &loadMainImageFromPixels);
  emscripten::function("addOverlayImageFromPixels", &addOverlayImageFromPixels);

  emscripten::function("setOpacity",      &setOpacity);
  emscripten::function("setRotation",     &setRotation);
  emscripten::function("setScale",        &setScale);

  emscripten::function("nextImage",       &nextImage);
  emscripten::function("prevImage",       &prevImage);
  emscripten::function("resetImage",      &resetImage);
  emscripten::function("saveImage",       &saveImage);

  emscripten::function("moveImageLeft",   &moveImageLeft);
  emscripten::function("moveImageRight",  &moveImageRight);
  emscripten::function("moveImageUp",     &moveImageUp);
  emscripten::function("moveImageDown",   &moveImageDown);
  emscripten::function("moveOverlayLeft", &moveOverlayLeft);
  emscripten::function("moveOverlayRight",&moveOverlayRight);
  emscripten::function("moveOverlayUp",   &moveOverlayUp);
  emscripten::function("moveOverlayDown", &moveOverlayDown);
}
