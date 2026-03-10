#include "atlasimageviewerweb.hpp"

// Emscripten / WebGL headers — only present when compiling with emcc.
#include <GLES2/gl2.h>

// AtlasImage wraps a cv::Mat and provides GetRawData() (RGBA bytes) and
// GetImage() (cv::Mat*) — used for both uploading to WebGL and future
// OpenCV-based pixel manipulations.
#include "AtlasImage/image.hpp"
#include <opencv2/imgproc.hpp>

#include <cmath>
#include <array>
#include <numbers>
#include <algorithm>

namespace
{
  constexpr float kPi = static_cast<float>(std::numbers::pi);

  // ---------------------------------------------------------------------------
  // GLSL ES 1.00 shaders (compatible with WebGL 1 and WebGL 2)
  // ---------------------------------------------------------------------------
  constexpr const char* kVertexShaderSrc = R"(
    attribute vec2 a_position;
    attribute vec2 a_texCoord;

    uniform mat3 u_transform;

    varying vec2 v_texCoord;

    void main()
    {
      vec3 pos = u_transform * vec3(a_position, 1.0);
      gl_Position = vec4(pos.xy, 0.0, 1.0);
      v_texCoord  = a_texCoord;
    }
  )";

  constexpr const char* kFragmentShaderSrc = R"(
    precision mediump float;

    uniform sampler2D u_texture;
    uniform float     u_opacity;

    varying vec2 v_texCoord;

    void main()
    {
      vec4 color  = texture2D(u_texture, v_texCoord);
      gl_FragColor = vec4(color.rgb, color.a * u_opacity);
    }
  )";

  // Full-screen quad: position (x,y) + texCoord (u,v), two triangles.
  constexpr float kQuadVertices[] = {
  //  x      y      u     v
    -1.0f, -1.0f,  0.0f, 0.0f,
     1.0f, -1.0f,  1.0f, 0.0f,
     1.0f,  1.0f,  1.0f, 1.0f,
    -1.0f,  1.0f,  0.0f, 1.0f,
  };

  auto CompileShader(GLenum type, const char* src) -> GLuint
  {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    return shader;
  }

  // Build a column-major 3x3 transform matrix encoding scale + rotation + translation.
  // Aspect correction is applied so rotating the quad stays circular on non-square viewports.
  auto BuildTransform(float scaleX, float scaleY,
                      float panX,   float panY,
                      float rotationDegrees,
                      float viewportAspect) -> std::array<float, 9>
  {
    const float rad  = rotationDegrees * (kPi / 180.0f);
    const float cosA = std::cos(rad);
    const float sinA = std::sin(rad);

    // Rotate in square space (aspect-corrected) then scale back out.
    // Column-major layout expected by glUniformMatrix3fv.
    std::array<float, 9> m = {
       cosA * scaleX / viewportAspect,  sinA * scaleY,                  0.0f,
      -sinA * scaleX,                   cosA * scaleY * viewportAspect, 0.0f,
       panX,                            panY,                           1.0f,
    };
    return m;  // NOTE: copy elided; caller receives by value via structured binding.
  }
}

namespace AtlasImageViewerWeb
{
  // ---------------------------------------------------------------------------
  // Destructor
  // ---------------------------------------------------------------------------
  ImageViewer::~ImageViewer()
  {
    if(m_baseTexture)   glDeleteTextures(1, &m_baseTexture);
    for(auto tex : m_overlayTextures) glDeleteTextures(1, &tex);
    if(m_vbo)           glDeleteBuffers(1, &m_vbo);
    if(m_shaderProgram) glDeleteProgram(m_shaderProgram);
  }

  // ---------------------------------------------------------------------------
  // Initialization
  // ---------------------------------------------------------------------------
  auto ImageViewer::InitializeGL(int canvasWidth, int canvasHeight) -> void
  {
    m_viewportWidth  = canvasWidth;
    m_viewportHeight = canvasHeight;

    CompileShaders();

    // Upload the static quad geometry.
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kQuadVertices), kQuadVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glViewport(0, 0, canvasWidth, canvasHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_initialized = true;
  }

  auto ImageViewer::Resize(int width, int height) -> void
  {
    m_viewportWidth  = width;
    m_viewportHeight = height;
    glViewport(0, 0, width, height);
  }

  // ---------------------------------------------------------------------------
  // Rendering
  // ---------------------------------------------------------------------------
  auto ImageViewer::Paint() -> void
  {
    if(!m_initialized) return;

    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(m_shaderProgram);

    // Draw base image (opacity 1.0, no overlay blending).
    if(m_baseTexture)
    {
      const float baseAspect = (m_baseHeight > 0)
        ? static_cast<float>(m_baseWidth) / static_cast<float>(m_baseHeight)
        : 1.0f;
      const float vpAspect = (m_viewportHeight > 0)
        ? static_cast<float>(m_viewportWidth) / static_cast<float>(m_viewportHeight)
        : 1.0f;

      // Fit base image to viewport preserving aspect ratio.
      float sx = 1.0f, sy = 1.0f;
      if(baseAspect > vpAspect)  sy = vpAspect / baseAspect;
      else                       sx = baseAspect / vpAspect;

      DrawQuad(m_baseTexture,
               static_cast<float>(sx * m_scale),
               static_cast<float>(sy * m_scale),
               static_cast<float>(m_basePanX),
               static_cast<float>(m_basePanY),
               static_cast<float>(m_rotationDegrees),
               1.0f);
    }

    // Draw overlay (current atlas match, blended).
    if(!m_overlayTextures.empty())
    {
      const GLuint overlayTex = m_overlayTextures[m_overlayIndex];
      DrawQuad(overlayTex,
               static_cast<float>(m_overlayScale),
               static_cast<float>(m_overlayScale),
               static_cast<float>(m_overlayPanX),
               static_cast<float>(m_overlayPanY),
               static_cast<float>(m_rotationDegrees),
               static_cast<float>(m_opacity));
    }
  }

  // ---------------------------------------------------------------------------
  // IImageViewer dispatch — mirrors the Qt ImageViewer::HandleStateUpdate logic
  // ---------------------------------------------------------------------------
  auto ImageViewer::HandleStateUpdate(AtlasCommon::AtlasImageViewerState state,
                                      std::string_view imageInformation) -> void
  {
    using S = AtlasCommon::AtlasImageViewerState;
    switch(state)
    {
      case S::Idle:             break;
      case S::LoadImage:        LoadMainImage(imageInformation);       break;
      case S::AddImage:         AddOverlayImage(imageInformation);     break;
      case S::AddImageWithWeight: AddOverlayImage(imageInformation);   break; // weight ignored for now
      case S::NextImage:        NextImage();                           break;
      case S::PreviousImage:    PrevImage();                           break;
      case S::UpdateRenderer:   Paint();                               break;
      // Progress popup states are no-ops in the web build — the HTML UI
      // manages its own loading indicator.
      case S::ConstructPopup:   break;
      case S::DisplayPopup:     break;
      case S::DestroyPopup:     break;
      default:                  break;
    }
  }

  auto ImageViewer::HandleStateUpdate(AtlasCommon::AtlasImageViewerState state,
                                      std::string_view /*mainLabelText*/,
                                      std::string_view /*progressBarTextFormat*/) -> void
  {
    // Popup construction is handled entirely by the HTML/TypeScript UI.
    (void)state;
  }

  auto ImageViewer::HandleStateUpdate(AtlasCommon::AtlasImageViewerState state,
                                      int value) -> void
  {
    using S = AtlasCommon::AtlasImageViewerState;
    switch(state)
    {
      case S::SliderUpdated:               SetOpacity(static_cast<double>(value) / 100.0); break;
      case S::RotateImage:                 RotateImage(static_cast<double>(value) / 1000.0); break;
      case S::ScaleImage:                  ScaleImage(value); break;
      case S::SetMaximumProgressBarValue:  break; // handled by HTML UI
      case S::UpdateProgressBarValue:      break; // handled by HTML UI
      default:                             break;
    }
  }

  // ---------------------------------------------------------------------------
  // Direct control
  // ---------------------------------------------------------------------------
  auto ImageViewer::SetOpacity(double opacity) -> void
  {
    m_opacity = std::clamp(opacity, 0.0, 1.0);
    Paint();
  }

  auto ImageViewer::SetRotation(double degrees) -> void
  {
    m_rotationDegrees = degrees;
    Paint();
  }

  auto ImageViewer::SetScale(double scale) -> void
  {
    m_scale = std::max(0.01, scale);
    Paint();
  }

  auto ImageViewer::SetOverlayScale(double scale) -> void
  {
    m_overlayScale = std::max(0.01, scale);
    Paint();
  }

  auto ImageViewer::NextImage() -> void
  {
    if(m_overlayTextures.empty()) return;
    m_overlayIndex = (m_overlayIndex + 1) % m_overlayTextures.size();
    Paint();
  }

  auto ImageViewer::PrevImage() -> void
  {
    if(m_overlayTextures.empty()) return;
    m_overlayIndex = (m_overlayIndex == 0)
      ? m_overlayTextures.size() - 1
      : m_overlayIndex - 1;
    Paint();
  }

  auto ImageViewer::ResetImage() -> void
  {
    m_rotationDegrees = 0.0;
    m_basePanX = m_basePanY = 0.0;
    m_overlayPanX = m_overlayPanY = 0.0;
    m_scale = m_overlayScale = 1.0;
    Paint();
  }

  auto ImageViewer::SaveImage() -> void
  {
    // Trigger a canvas download via a small JS snippet injected at runtime.
    // The actual emscripten_run_script call will live in atlasimager_web.cpp
    // once the entry point is wired up, so this is a placeholder.
  }

  // Pan helpers — step size matches the Qt build's 0.01 NDC increment.
  auto ImageViewer::MoveImageLeft()    -> void { m_basePanX    -= 0.01; Paint(); }
  auto ImageViewer::MoveImageRight()   -> void { m_basePanX    += 0.01; Paint(); }
  auto ImageViewer::MoveImageUp()      -> void { m_basePanY    += 0.01; Paint(); }
  auto ImageViewer::MoveImageDown()    -> void { m_basePanY    -= 0.01; Paint(); }
  auto ImageViewer::MoveOverlayLeft()  -> void { m_overlayPanX -= 0.01; Paint(); }
  auto ImageViewer::MoveOverlayRight() -> void { m_overlayPanX += 0.01; Paint(); }
  auto ImageViewer::MoveOverlayUp()    -> void { m_overlayPanY += 0.01; Paint(); }
  auto ImageViewer::MoveOverlayDown()  -> void { m_overlayPanY -= 0.01; Paint(); }

  // ---------------------------------------------------------------------------
  // Private helpers
  // ---------------------------------------------------------------------------
  auto ImageViewer::CompileShaders() -> void
  {
    const GLuint vert = CompileShader(GL_VERTEX_SHADER,   kVertexShaderSrc);
    const GLuint frag = CompileShader(GL_FRAGMENT_SHADER, kFragmentShaderSrc);

    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vert);
    glAttachShader(m_shaderProgram, frag);
    glLinkProgram(m_shaderProgram);

    glDeleteShader(vert);
    glDeleteShader(frag);
  }

  auto ImageViewer::UploadTexture(const unsigned char* rgba,
                                  int width, int height) -> GLuint
  {
    GLuint id{};
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return id;
  }

  auto ImageViewer::DeleteTexture(GLuint& textureId) -> void
  {
    if(textureId) { glDeleteTextures(1, &textureId); textureId = 0; }
  }

  auto ImageViewer::DrawQuad(GLuint textureId,
                              float scaleX, float scaleY,
                              float panX,   float panY,
                              float rotation,
                              float opacity) -> void
  {
    const float vpAspect = (m_viewportHeight > 0)
      ? static_cast<float>(m_viewportWidth) / static_cast<float>(m_viewportHeight)
      : 1.0f;

    // Build transform — see anonymous namespace helper.
    const float rad  = rotation * (kPi / 180.0f);
    const float cosA = std::cos(rad);
    const float sinA = std::sin(rad);
    const float transform[9] = {
       cosA * scaleX / vpAspect,  sinA * scaleY,              0.0f,
      -sinA * scaleX,             cosA * scaleY * vpAspect,   0.0f,
       panX,                      panY,                        1.0f,
    };

    const GLint uTransform = glGetUniformLocation(m_shaderProgram, "u_transform");
    const GLint uTexture   = glGetUniformLocation(m_shaderProgram, "u_texture");
    const GLint uOpacity   = glGetUniformLocation(m_shaderProgram, "u_opacity");
    const GLint aPosition  = glGetAttribLocation(m_shaderProgram,  "a_position");
    const GLint aTexCoord  = glGetAttribLocation(m_shaderProgram,  "a_texCoord");

    glUniformMatrix3fv(uTransform, 1, GL_FALSE, transform);
    glUniform1i(uTexture, 0);
    glUniform1f(uOpacity, opacity);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    constexpr int stride = 4 * sizeof(float);
    glEnableVertexAttribArray(aPosition);
    glVertexAttribPointer(aPosition, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const void*>(0));
    glEnableVertexAttribArray(aTexCoord);
    glVertexAttribPointer(aTexCoord, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const void*>(2 * sizeof(float)));

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisableVertexAttribArray(aPosition);
    glDisableVertexAttribArray(aTexCoord);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  // Convert a cv::Mat (any depth/channel count) to a packed RGBA buffer
  // suitable for glTexImage2D, then upload it.
  auto ImageViewer::UploadFromAtlasImage(AtlasImage::Image& img,
                                         int& outWidth, int& outHeight) -> GLuint
  {
    const cv::Mat* src = img.GetImage();
    if(!src || src->empty()) return 0;

    cv::Mat rgba;
    if(src->channels() == 1)
      cv::cvtColor(*src, rgba, cv::COLOR_GRAY2RGBA);
    else if(src->channels() == 3)
      cv::cvtColor(*src, rgba, cv::COLOR_BGR2RGBA);
    else
      rgba = *src; // already RGBA

    // Flip vertically so the image is right-side up in OpenGL's bottom-left origin.
    cv::Mat flipped;
    cv::flip(rgba, flipped, 0);

    outWidth  = flipped.cols;
    outHeight = flipped.rows;
    return UploadTexture(flipped.data, outWidth, outHeight);
  }

  auto ImageViewer::LoadMainImage(std::string_view imagePath) -> void
  {
    auto img = AtlasImage::Image{imagePath};
    DeleteTexture(m_baseTexture);
    m_baseTexture = UploadFromAtlasImage(img, m_baseWidth, m_baseHeight);
    Paint();
  }

  auto ImageViewer::AddOverlayImage(std::string_view imagePath) -> void
  {
    auto img = AtlasImage::Image{imagePath};
    int w{}, h{};
    GLuint tex = UploadFromAtlasImage(img, w, h);
    if(tex) m_overlayTextures.push_back(tex);
    Paint();
  }

  auto ImageViewer::LoadMainImageFromPixels(const unsigned char* data,
                                            int width, int height, int channels) -> void
  {
    const int cvType = (channels == 1) ? CV_8UC1
                     : (channels == 3) ? CV_8UC3
                     : CV_8UC4;
    auto img = AtlasImage::Image{"user_image", data, width, height, cvType};
    DeleteTexture(m_baseTexture);
    m_baseTexture = UploadFromAtlasImage(img, m_baseWidth, m_baseHeight);
    Paint();
  }

  auto ImageViewer::AddOverlayImageFromPixels(const unsigned char* data,
                                              int width, int height, int channels) -> void
  {
    const int cvType = (channels == 1) ? CV_8UC1
                     : (channels == 3) ? CV_8UC3
                     : CV_8UC4;
    auto img = AtlasImage::Image{"overlay_image", data, width, height, cvType};
    int w{}, h{};
    GLuint tex = UploadFromAtlasImage(img, w, h);
    if(tex) m_overlayTextures.push_back(tex);
    Paint();
  }

  auto ImageViewer::RotateImage(double degrees) -> void
  {
    m_rotationDegrees = degrees;
    Paint();
  }

  auto ImageViewer::ScaleImage(int percentValue) -> void
  {
    const int clamped = std::clamp(percentValue, 1, 300);
    m_scale = static_cast<double>(clamped) / 100.0;
    Paint();
  }
}
