#pragma once

#include "AtlasCommon/atlasenums.hpp"
#include "AtlasCommon/iimageviewer.hpp"
#include "AtlasImage/image.hpp"

#include <string_view>
#include <vector>
#include <memory>

// WebGL / GLES2 types — provided by Emscripten's GLES2/gl2.h at compile time.
// The aliases below keep the header self-contained during IDE parsing before
// the Emscripten toolchain is active.
#ifndef GL_ES_VERSION_2_0
using GLuint = unsigned int;
using GLint  = int;
#endif

namespace AtlasImageViewerWeb
{
  /*
  * Web-native image viewer that renders using WebGL ES 2.0 shaders.
  * No Qt types, no fixed-function OpenGL — safe to compile with Emscripten.
  *
  * Implements AtlasCommon::IImageViewer so AtlasMessenger can dispatch to it
  * identically to the Qt desktop build.
  */
  class ImageViewer : public AtlasCommon::IImageViewer
  {
  public:
    ImageViewer() = default;
    ~ImageViewer() override;

    // Called once from the WASM entry point after the WebGL context is ready.
    auto InitializeGL(int canvasWidth, int canvasHeight) -> void;

    // Called each frame via JS requestAnimationFrame (or on demand after state change).
    auto Paint() -> void;

    // Resize the GL viewport when the canvas is resized.
    auto Resize(int width, int height) -> void;

    // -----------------------------------------------------------------------
    // IImageViewer — dispatches the same state machine as the Qt build
    // -----------------------------------------------------------------------
    auto HandleStateUpdate(AtlasCommon::AtlasImageViewerState state,
                           std::string_view imageInformation = "") -> void override;

    auto HandleStateUpdate(AtlasCommon::AtlasImageViewerState state,
                           std::string_view mainLabelText,
                           std::string_view progressBarTextFormat) -> void override;

    auto HandleStateUpdate(AtlasCommon::AtlasImageViewerState state,
                           int value) -> void override;

    // -----------------------------------------------------------------------
    // Direct control — also callable from WASM embind bindings
    // -----------------------------------------------------------------------
    auto SetOpacity(double opacity)       -> void;
    auto SetRotation(double degrees)      -> void;
    auto SetScale(double scale)           -> void;
    auto SetOverlayScale(double scale)    -> void;

    auto MoveImageLeft()    -> void;
    auto MoveImageRight()   -> void;
    auto MoveImageUp()      -> void;
    auto MoveImageDown()    -> void;

    auto MoveOverlayLeft()  -> void;
    auto MoveOverlayRight() -> void;
    auto MoveOverlayUp()    -> void;
    auto MoveOverlayDown()  -> void;

    auto NextImage()        -> void;
    auto PrevImage()        -> void;
    auto ResetImage()       -> void;

    // Triggers a browser download of the current canvas contents as a PNG.
    auto SaveImage()        -> void;

    // Load images from raw decoded pixel data (WASM path — no cv::imread).
    // data must be packed 8-bit pixels: width * height * channels bytes.
    // channels: 1 = grayscale, 3 = RGB, 4 = RGBA
    auto LoadMainImageFromPixels(const unsigned char* data,
                                 int width, int height, int channels) -> void;
    auto AddOverlayImageFromPixels(const unsigned char* data,
                                   int width, int height, int channels) -> void;

  private:
    // -----------------------------------------------------------------------
    // GL state
    // -----------------------------------------------------------------------
    GLuint m_shaderProgram{0};
    GLuint m_vbo{0};

    // Base image texture (the XRay / user image)
    GLuint m_baseTexture{0};
    int    m_baseWidth{0};
    int    m_baseHeight{0};

    // Overlay textures (atlas model matches), cycled via Next/Prev
    std::vector<GLuint> m_overlayTextures;
    std::size_t         m_overlayIndex{0};

    // Viewport
    int m_viewportWidth{0};
    int m_viewportHeight{0};

    // Transform state — mirrors the Qt build's member variables
    double m_opacity{0.75};
    double m_rotationDegrees{0.0};
    double m_scale{1.0};
    double m_overlayScale{1.0};
    double m_basePanX{0.0};
    double m_basePanY{0.0};
    double m_overlayPanX{0.0};
    double m_overlayPanY{0.0};

    bool m_initialized{false};

    // -----------------------------------------------------------------------
    // Internal helpers
    // -----------------------------------------------------------------------
    auto CompileShaders()                                           -> void;
    auto UploadTexture(const unsigned char* rgba,
                       int width, int height)                       -> GLuint;
    auto DeleteTexture(GLuint& textureId)                           -> void;
    auto DrawQuad(GLuint textureId,
                  float scaleX, float scaleY,
                  float panX,   float panY,
                  float rotation,
                  float opacity)                                    -> void;

    auto LoadMainImage(std::string_view imagePath)                  -> void;
    auto AddOverlayImage(std::string_view imagePath)                -> void;
    auto UploadFromAtlasImage(AtlasImage::Image& img,
                              int& outWidth, int& outHeight)        -> GLuint;
    auto RotateImage(double degrees)                                -> void;
    auto ScaleImage(int percentValue)                               -> void;
  };
}
