#include "atlasimageviewer.hpp"

#include "AtlasLogger/atlaslogger.hpp"
#include "AtlasMessenger/atlasmessenger.hpp"


#include <ranges>
#include <print>
#include <numbers>
#include <expected>
#include <charconv>
#include <algorithm>

#include <QImage>
#include <QKeyEvent>
#include <QFileDialog>

#include <QColor>
#include <QMatrix4x4>

namespace AtlasImageViewer
{

  static AtlasLogger::Logger m_logger{std::filesystem::current_path().string() + "/Logs/ImageViewer.log", "AtlasImageViewer::ImageViewer"};

  enum class ImageViewerError : std::uint8_t
  {
    ImageLoadFailed,
    TextureCreationFailed,
    FrameBufferCreationFailed
  };

  enum class StringParseResult : std::uint8_t
  {
    DelimiterNotFound,
    ConversionFailed
  };

  auto ParseImageInformation(const std::string_view imageInformation) -> std::expected<std::pair<std::string_view, double>, StringParseResult>
  {
    auto colonPos = imageInformation.rfind(':');
    if(colonPos != std::string::npos)
    {
      auto imagePath = imageInformation.substr(0, colonPos);
      auto weight = double{0.0};
      if(std::from_chars(imageInformation.data() + colonPos + 1, imageInformation.data() + imageInformation.size(), weight).ec != std::errc{})
      {
        m_logger.Log(AtlasLogger::LogLevel::Error, "Failed to convert weight from string to double: {}", imageInformation.substr(colonPos + 1));
        return std::unexpected(StringParseResult::ConversionFailed);
      }

      return std::make_pair(imagePath, weight);
    }
    m_logger.Log(AtlasLogger::LogLevel::Error, "Delimiter ':' not found in image information string: {}", imageInformation);
    return std::unexpected(StringParseResult::DelimiterNotFound); 
  }

  namespace
  {
    struct Rgba
    {
      std::uint8_t r{};
      std::uint8_t g{};
      std::uint8_t b{};
      std::uint8_t a{};
    };

    auto GetPixelRgba8888(const QImage& image, const int x, const int y) -> Rgba
    {
      const auto* scan = image.constScanLine(y);
      const auto* px = scan + (x * 4);
      return Rgba{
        static_cast<std::uint8_t>(px[0]),
        static_cast<std::uint8_t>(px[1]),
        static_cast<std::uint8_t>(px[2]),
        static_cast<std::uint8_t>(px[3])
      };
    }

    auto SetPixelAlphaRgba8888(QImage& image, const int x, const int y, const std::uint8_t alpha) -> void
    {
      auto* scan = image.scanLine(y);
      auto* px = scan + (x * 4);
      px[3] = alpha;
    }

    [[maybe_unused]] auto EstimateCornerKeyColorRgba8888(const QImage& image) -> QColor
    {
      // Average a small block from each corner; robust against small artifacts.
      const int w = image.width();
      const int h = image.height();
      if(w <= 0 || h <= 0)
        return QColor{0, 0, 0};

      const int block = std::min({5, w, h});
      std::uint64_t sumR = 0, sumG = 0, sumB = 0;
      std::uint64_t count = 0;

      auto accumulateBlock = [&](const int startX, const int startY)
      {
        for(int y = startY; y < startY + block; ++y)
        {
          for(int x = startX; x < startX + block; ++x)
          {
            const auto px = GetPixelRgba8888(image, x, y);
            sumR += px.r;
            sumG += px.g;
            sumB += px.b;
            ++count;
          }
        }
      };

      accumulateBlock(0, 0);
      accumulateBlock(w - block, 0);
      accumulateBlock(0, h - block);
      accumulateBlock(w - block, h - block);

      if(count == 0)
        return QColor{0, 0, 0};

      return QColor{
        static_cast<int>(sumR / count),
        static_cast<int>(sumG / count),
        static_cast<int>(sumB / count)
      };
    }

    auto ApplyChromaKeyAlpha(QImage& image, const QColor keyColor, const int tolerance, const int feather) -> void
    {
      if(image.isNull())
        return;

      if(image.format() != QImage::Format_RGBA8888)
        image = image.convertToFormat(QImage::Format_RGBA8888);

      const int w = image.width();
      const int h = image.height();

      const int tol = std::max(0, tolerance);
      const int fea = std::max(0, feather);

      for(int y = 0; y < h; ++y)
      {
        for(int x = 0; x < w; ++x)
        {
          const auto px = GetPixelRgba8888(image, x, y);
          const int dr = std::abs(static_cast<int>(px.r) - keyColor.red());
          const int dg = std::abs(static_cast<int>(px.g) - keyColor.green());
          const int db = std::abs(static_cast<int>(px.b) - keyColor.blue());
          const int dist = dr + dg + db;

          std::uint8_t outA = 255;
          if(dist <= tol)
          {
            outA = 0;
          }
          else if(fea > 0 && dist < (tol + fea))
          {
            const float t = static_cast<float>(dist - tol) / static_cast<float>(fea);
            outA = static_cast<std::uint8_t>(std::clamp(t, 0.0f, 1.0f) * 255.0f);
          }

          SetPixelAlphaRgba8888(image, x, y, outA);
        }
      }
    }

    auto ApplyBlackBackgroundAlpha(QImage& image, const int threshold, const int feather) -> void
    {
      if(image.isNull())
        return;

      if(image.format() != QImage::Format_RGBA8888)
        image = image.convertToFormat(QImage::Format_RGBA8888);

      const int w = image.width();
      const int h = image.height();

      const int thr = std::max(0, threshold);
      const int fea = std::max(0, feather);

      for(int y = 0; y < h; ++y)
      {
        for(int x = 0; x < w; ++x)
        {
          const auto px = GetPixelRgba8888(image, x, y);
          const int maxChannel = std::max({static_cast<int>(px.r), static_cast<int>(px.g), static_cast<int>(px.b)});

          std::uint8_t outA = 255;
          if(maxChannel <= thr)
          {
            outA = 0;
          }
          else if(fea > 0 && maxChannel < (thr + fea))
          {
            const float t = static_cast<float>(maxChannel - thr) / static_cast<float>(fea);
            outA = static_cast<std::uint8_t>(std::clamp(t, 0.0f, 1.0f) * 255.0f);
          }

          SetPixelAlphaRgba8888(image, x, y, outA);
        }
      }
    }
  }

  auto ImageViewer::SetOverlayLuminanceThreshold(double threshold255) -> void
  {
    m_overlayLuminanceThreshold = std::clamp(threshold255, 0.0, 255.0);
    this->update();
  }

  auto ImageViewer::SetOverlayLuminanceFeather(double feather255) -> void
  {
    m_overlayLuminanceFeather = std::clamp(feather255, 0.0, 255.0);
    this->update();
  }

  auto ImageViewer::SetOverlayBrightness(double brightness) -> void
  {
    m_overlayBrightness = std::max(0.0, brightness);
    this->update();
  }

  auto ImageViewer::SetOverlayTint(const QVector3D& rgb01) -> void
  {
    m_overlayTint = QVector3D{
      std::clamp(rgb01.x(), 0.0f, 1.0f),
      std::clamp(rgb01.y(), 0.0f, 1.0f),
      std::clamp(rgb01.z(), 0.0f, 1.0f)
    };
    this->update();
  }

  auto ImageViewer::EnsureQuadPipeline() -> void
  {
    if(m_quadPipelineReady)
      return;

    // Minimal textured-quad shader with transform + optional luminance discard.
    static constexpr const char* kVert = R"(
      #version 330 core
      layout(location = 0) in vec2 a_pos;
      layout(location = 1) in vec2 a_uv;
      uniform mat4 u_mvp;
      out vec2 v_uv;
      void main() {
        v_uv = a_uv;
        gl_Position = u_mvp * vec4(a_pos, 0.0, 1.0);
      }
    )";

    static constexpr const char* kFrag = R"(
      #version 330 core
      in vec2 v_uv;
      uniform sampler2D u_tex;
      uniform vec4 u_color;      // rgb tint + opacity multiplier in a
      uniform float u_brightness;
      uniform bool u_useLumaKey;
      // UI-friendly units (0..255). We normalize to 0..1 for the luminance compare.
      uniform float u_lumaThreshold255;
      uniform float u_lumaFeather255;
      out vec4 FragColor;
      void main() {
        vec4 texel = texture(u_tex, v_uv);
        vec3 rgb = texel.rgb * u_brightness;
        float a = texel.a;

        if(u_useLumaKey) {
          float lum = dot(texel.rgb, vec3(0.2126, 0.7152, 0.0722));
          float thr = clamp(u_lumaThreshold255 / 255.0, 0.0, 1.0);
          float fea = clamp(u_lumaFeather255 / 255.0, 0.0, 1.0);
          float edge = smoothstep(thr, thr + max(fea, 1e-6), lum);
          a *= edge;
          if(a <= 0.001) discard;
        }

        vec4 outColor = vec4(rgb, a) * u_color;
        FragColor = outColor;
      }
    )";

    m_quadProgram = std::make_unique<QOpenGLShaderProgram>();
    if(!m_quadProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, kVert))
    {
      m_logger.Log(AtlasLogger::LogLevel::Error, "Quad vertex shader compile failed: {}", m_quadProgram->log().toStdString());
      m_quadProgram.reset();
      return;
    }
    if(!m_quadProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, kFrag))
    {
      m_logger.Log(AtlasLogger::LogLevel::Error, "Quad fragment shader compile failed: {}", m_quadProgram->log().toStdString());
      m_quadProgram.reset();
      return;
    }
    if(!m_quadProgram->link())
    {
      m_logger.Log(AtlasLogger::LogLevel::Error, "Quad program link failed: {}", m_quadProgram->log().toStdString());
      m_quadProgram.reset();
      return;
    }

    // Fullscreen quad (two triangles). Positions in NDC, UVs in [0,1].
    struct Vertex { float x, y, u, v; };
    static constexpr Vertex kVerts[] = {
      {-1.0f, -1.0f, 0.0f, 0.0f},
      { 1.0f, -1.0f, 1.0f, 0.0f},
      { 1.0f,  1.0f, 1.0f, 1.0f},
      {-1.0f, -1.0f, 0.0f, 0.0f},
      { 1.0f,  1.0f, 1.0f, 1.0f},
      {-1.0f,  1.0f, 0.0f, 1.0f}
    };

    if(!m_quadVbo.isCreated())
      m_quadVbo.create();
    m_quadVbo.bind();
    m_quadVbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    m_quadVbo.allocate(kVerts, static_cast<int>(sizeof(kVerts)));

    if(!m_quadVao.isCreated())
      m_quadVao.create();
    m_quadVao.bind();
    m_quadProgram->bind();

    // a_pos (location = 0)
    m_quadProgram->enableAttributeArray(0);
    m_quadProgram->setAttributeBuffer(0, GL_FLOAT, offsetof(Vertex, x), 2, sizeof(Vertex));
    // a_uv (location = 1)
    m_quadProgram->enableAttributeArray(1);
    m_quadProgram->setAttributeBuffer(1, GL_FLOAT, offsetof(Vertex, u), 2, sizeof(Vertex));

    m_quadProgram->release();
    m_quadVao.release();
    m_quadVbo.release();

    m_quadPipelineReady = true;
  }

  ImageViewer::ImageViewer() : m_textureId{0}
  {
    // std::ranges::fill(m_fbos, std::pair{nullptr, 0});

  }

  ImageViewer::~ImageViewer()
  {
    CleanUp();
    m_offscreensurface.reset();
    
    /*
    std::ranges::for_each(m_fbos, [](auto& fbo){
      if(fbo.first)
        fbo.first.reset();
    });
    */

    std::ranges::for_each(m_fbos, [](auto& fbo){
      if(fbo)
        fbo.reset();
    });

  }

  auto ImageViewer::RenderMainImage(const std::string_view imagePath) -> void
  {
    m_logger.Log(AtlasLogger::LogLevel::Info, "Loading image from path: {}", imagePath);
    this->makeCurrent();

    if(m_texture)
      CleanUp();

    auto image = QImage{QString::fromStdString(std::string{imagePath})}; 

    m_logger.Log(AtlasLogger::LogLevel::Info, "Image loaded with size: {}x{}", image.width(), image.height());

    auto glImage = image.convertToFormat(QImage::Format_RGBA8888).mirrored();

    // Keep the original pixels; black-background removal is done in the overlay shader now.

    m_logger.Log(AtlasLogger::LogLevel::Info, "Converted image to OpenGL format with size: {}x{}", glImage.width(), glImage.height());

    auto gl_funcs = this->context()->functions();
    auto texture = CreateTexture(glImage, gl_funcs);

    m_fbo = CreateFrameBuffer(image.size());

    m_logger.Log(AtlasLogger::LogLevel::Info, "Created FBO with size: {}x{}", m_fbo->size().width(), m_fbo->size().height());

    this->DrawToFBO(m_fbo.get(), gl_funcs, texture);

    gl_funcs->glDeleteTextures(1, &texture);

    this->update();
  }

  auto ImageViewer::DrawToFBO(QOpenGLFramebufferObject* fbo, QOpenGLFunctions* gl_funcs, const GLuint textureId) -> void
  {
    fbo->bind();
    glViewport(0, 0, fbo->size().width(), fbo->size().height());

    // Preserve transparency in the FBO so blending works as expected.
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId);

    // Render a textured quad
    glBegin(GL_QUADS);
      glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
      glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f, -1.0f);
      glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f,  1.0f);
      glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f,  1.0f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    fbo->release();
    glViewport(0, 0, this->width(), this->height());
  }

  auto ImageViewer::CreateImage(const std::string_view imagePath) -> QImage
  {
    auto image = QImage{QString::fromStdString(std::string{imagePath})};
    auto glImage = image.convertToFormat(QImage::Format_RGBA8888).mirrored();
    m_logger.Log(AtlasLogger::LogLevel::Info, "Created QImage from path: {} with size: {}x{}", imagePath, glImage.width(), glImage.height());
    return glImage;
  }

  auto ImageViewer::CreateTexture(const QImage& image, QOpenGLFunctions* gl_funcs) -> GLuint
  {
    auto textureId = GLuint{};

    gl_funcs->glGenTextures(1, &textureId);
    gl_funcs->glBindTexture(GL_TEXTURE_2D, textureId);
    gl_funcs->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.constBits());
    gl_funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl_funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl_funcs->glBindTexture(GL_TEXTURE_2D, 0);

    return textureId;
  }
  
  auto ImageViewer::CreateFrameBuffer(const QSize size) -> std::unique_ptr<QOpenGLFramebufferObject>
  {
    auto fboFormat = QOpenGLFramebufferObjectFormat{};
    fboFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    auto fbo = std::make_unique<QOpenGLFramebufferObject>(size, fboFormat);

    return fbo;
  }

  auto ImageViewer::AddFBOToVector(std::unique_ptr<QOpenGLFramebufferObject>&& fbo) -> void
  {
    m_fbos.emplace_back(std::move(fbo));
    m_logger.Log(AtlasLogger::LogLevel::Info, "Successfully added FBO to vector. Total now: {}", m_fbos.size());
  }

  /*
  auto ImageViewer::AddFBOToArray(std::unique_ptr<QOpenGLFramebufferObject>&& fbo, const double weight) -> void
  {
      m_fbos.emplace_back(std::move(fbo), weight);
      m_logger.Log(AtlasLogger::LogLevel::Info, "Successfully added FBO to array. Total now: {}", m_fbos.size());
//    if(m_fbos[0].first == nullptr)
//    {
//      m_fbos[0].first = std::move(fbo);
//      m_fbos[0].second = weight;
//    }
//    else
//    {
//      auto minWeight = std::ranges::min_element(m_fbos, [](const auto& lhs, const auto& rhs)
//      {
//        return lhs.second < rhs.second;
//      });
//
//      if (weight > minWeight->second)
//      {
//        auto minWeightIndex = std::distance(m_fbos.begin(), minWeight);
//        m_fbos[minWeightIndex].second = weight;
//        m_fbos[minWeightIndex].first = std::move(fbo);
//      }
//    }
//    std::println("Successfully added FBO to array");
  }
  */

  auto ImageViewer::AddImage(const std::string_view imagePath) -> void
  {
    auto image = CreateImage(imagePath);

    if(!m_offscreenContext || !m_offscreensurface)
    {
      m_logger.Log(AtlasLogger::LogLevel::Error, "Offscreen context/surface not initialized; cannot AddImage");
      return;
    }

    m_offscreenContext->makeCurrent(m_offscreensurface.get());
    auto gl_funcs = m_offscreenContext->functions();
    auto textureId = CreateTexture(image, gl_funcs);

    m_logger.Log(AtlasLogger::LogLevel::Info, "Created texture for image addition with ID: {}", textureId);

    auto fbo = CreateFrameBuffer(image.size());

    if(!fbo->isValid())
    {
      fbo.reset();
      gl_funcs->glDeleteTextures(1, &textureId);
    }

    m_logger.Log(AtlasLogger::LogLevel::Info, "Created FBO for image addition with size: {}x{}", fbo->size().width(), fbo->size().height());

    this->DrawToFBO(fbo.get(), gl_funcs, textureId);

    gl_funcs->glDeleteTextures(1, &textureId);

    m_offscreenContext->doneCurrent();

    this->AddFBOToVector(std::move(fbo));

    m_logger.Log(AtlasLogger::LogLevel::Info, "Adding FBO to Vector. Total now: {}", m_fbos.size());
  }


  // should this be a unique_ptr of an OpenCV Mat passed in by rvalue?
  // or should this be an array of images already sorted? 
  auto ImageViewer::AddImageWithWeight(const std::string_view imagePath, const double weight) -> void
  {
    auto image = CreateImage(imagePath);

    if(!m_offscreenContext || !m_offscreensurface)
    {
      m_logger.Log(AtlasLogger::LogLevel::Error, "Offscreen context/surface not initialized; cannot AddImageWithWeight");
      return;
    }

    m_offscreenContext->makeCurrent(m_offscreensurface.get());
    auto gl_funcs = m_offscreenContext->functions();
    auto textureId = CreateTexture(image, gl_funcs);

    m_logger.Log(AtlasLogger::LogLevel::Info, "Created texture for image addition with ID: {}", textureId);

    auto fbo = CreateFrameBuffer(image.size());

    if(!fbo->isValid())
    {
      fbo.reset();
      gl_funcs->glDeleteTextures(1, &textureId);
    }

    m_logger.Log(AtlasLogger::LogLevel::Info, "Created FBO for image addition with size: {}x{}", fbo->size().width(), fbo->size().height());

    this->DrawToFBO(fbo.get(), gl_funcs, textureId);

    gl_funcs->glDeleteTextures(1, &textureId);

    m_offscreenContext->doneCurrent();

    //this->AddFBOToArray(std::move(fbo), weight);

    m_logger.Log(AtlasLogger::LogLevel::Info, "Adding FBO to Array. Total now: {}", m_fbos.size());
  }

  auto ImageViewer::ProcessAddImage(const std::string_view imageInformation) -> void
  {
    this->AddImage(imageInformation);
    //TODO: Blend and render the images.
  }

  auto ImageViewer::ProcessAddImageWithWeight(const std::string_view imageInformation) -> void
  {
    // Parse imageInformation to get image path and weight
    auto [imagePath, weight] = ParseImageInformation(imageInformation).value_or(std::pair{"", 0.0});

    if(imagePath.empty() && weight == 0.0)
    {
      m_logger.Log(AtlasLogger::LogLevel::Error, "Failed to parse image information: {}", imageInformation);
      return;
    }

    this->AddImageWithWeight(imagePath, weight);
  }

  auto ImageViewer::HandleStateUpdate(const AtlasCommon::AtlasImageViewerState state, const std::string_view imageInformation) -> void
  {
    switch(state)
    {
      case AtlasCommon::AtlasImageViewerState::Idle:
        // Do nothing
        return;
      case AtlasCommon::AtlasImageViewerState::DisplayPopup:
        m_logger.Log(AtlasLogger::LogLevel::Info, "Displaying loading popup");
        emit this->DisplayLoadingModelPopupSignal();
        break;
      case AtlasCommon::AtlasImageViewerState::DestroyPopup:
        m_logger.Log(AtlasLogger::LogLevel::Info, "Destroying loading popup");
        emit this->DestroyLoadingModelPopupSignal();
        break;
      case AtlasCommon::AtlasImageViewerState::AddImage:
        m_logger.Log(AtlasLogger::LogLevel::Info, "Adding an image");
        this->ProcessAddImage(imageInformation);
        break;
      case AtlasCommon::AtlasImageViewerState::AddImageWithWeight:
        m_logger.Log(AtlasLogger::LogLevel::Info, "Adding an image with weight");
        this->ProcessAddImageWithWeight(imageInformation);
        break;
      case AtlasCommon::AtlasImageViewerState::UpdateRenderer:
        m_logger.Log(AtlasLogger::LogLevel::Info, "Updating renderer");
        this->update();
        break;
      case AtlasCommon::AtlasImageViewerState::LoadImage:
        m_logger.Log(AtlasLogger::LogLevel::Info, "Loading an image");
        this->RenderMainImage(imageInformation);
        break;
      case AtlasCommon::AtlasImageViewerState::NextImage:
        m_logger.Log(AtlasLogger::LogLevel::Info, "Next Image requested");
        this->OnNextButtonPressed();
        break;
      case AtlasCommon::AtlasImageViewerState::PreviousImage:
        m_logger.Log(AtlasLogger::LogLevel::Info, "Previous Image requested");
        this->OnPrevButtonPressed();
        break;
      case AtlasCommon::AtlasImageViewerState::RotateImage:
        m_logger.Log(AtlasLogger::LogLevel::Info, "Rotate Image requested");
        this->RotateImage(std::string{imageInformation});
        break;
      default:
        break;
    }
  }

  auto ImageViewer::HandleStateUpdate(const AtlasCommon::AtlasImageViewerState state, const std::string_view mainLabelText, const std::string_view progressBarTextformat) -> void
  {
    switch(state)
    {
      case AtlasCommon::AtlasImageViewerState::ConstructPopup:
        m_logger.Log(AtlasLogger::LogLevel::Info, "Constructing Popup");
        emit CreateLoadingModelPopupSignal(mainLabelText, progressBarTextformat);
        break;
      default:
        break;
    }
  }

  auto ImageViewer::HandleStateUpdate(const AtlasCommon::AtlasImageViewerState state, const int value) -> void
  {
    switch(state)
    {
      case AtlasCommon::AtlasImageViewerState::SetMaximumProgressBarValue:
        m_logger.Log(AtlasLogger::LogLevel::Info, "Set Maximum Progress Bar Value requested: {}", value);
        emit SetMaximumProgressBarValueSignal(value);
        break;
      case AtlasCommon::AtlasImageViewerState::UpdateProgressBarValue:
        m_logger.Log(AtlasLogger::LogLevel::Info, "Update Progress Bar Value requested: {}", value);
        emit UpdateProgressBarValueSignal(value);
        break;
      case AtlasCommon::AtlasImageViewerState::SliderUpdated:
        m_logger.Log(AtlasLogger::LogLevel::Info, "Slider Updated requested");
        this->OnSliderUpdated(static_cast<double>(value) / 100.0);
        break;
      case AtlasCommon::AtlasImageViewerState::LuminanceThresholdUpdated:
        m_logger.Log(AtlasLogger::LogLevel::Info, "Luminance Threshold Update requested");
        this->SetOverlayLuminanceThreshold(static_cast<double>(value));
        break;
      case AtlasCommon::AtlasImageViewerState::LuminanceFeatherUpdated:
        m_logger.Log(AtlasLogger::LogLevel::Info, "Luminance Feather Update requested");
        this->SetOverlayLuminanceFeather(static_cast<double>(value));
        break;
      case AtlasCommon::AtlasImageViewerState::BrightnessUpdated:
        m_logger.Log(AtlasLogger::LogLevel::Info, "Brightness Update requested");
        this->SetOverlayBrightness(static_cast<double>(value) / 100.0);
        break;
      default:
        break;
    }
  }

  auto ImageViewer::initializeGL() -> void
  {
    initializeOpenGLFunctions();

    // Shader pipeline uses the window context
    this->makeCurrent();
    EnsureQuadPipeline();
    m_offscreensurface = std::make_unique<QOffscreenSurface>();
    m_offscreensurface->setFormat(this->context()->format());
    m_offscreensurface->create();

    m_offscreenContext = std::make_unique<QOpenGLContext>();
    m_offscreenContext->setFormat(this->context()->format());
    m_offscreenContext->setShareContext(this->context());
    if(!m_offscreenContext->create())
    {
      m_logger.Log(AtlasLogger::LogLevel::Error, "Failed to create persistent offscreen OpenGL context");
      m_offscreenContext.reset();
      return;
    }

    if(!m_offscreenContext->makeCurrent(m_offscreensurface.get()))
    {
      m_logger.Log(AtlasLogger::LogLevel::Error, "Failed to make persistent offscreen context current");
      m_offscreenContext.reset();
      return;
    }
    m_offscreenContext->doneCurrent();
  }

  auto ImageViewer::resizeGL(int w, int h) -> void
  {
    glViewport(0, 0, w, h);
  }

  auto ImageViewer::paintGL() -> void
  {
    EnsureQuadPipeline();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if(!m_quadPipelineReady)
      return;

    // Before drawing FBOs
    auto baseScaleX = 1.0f;
    auto baseScaleY = 1.0f;
    auto overlayScaleX = 1.0f;
    auto overlayScaleY = 1.0f;
    const auto* baseFbo = (!m_fbos.empty() && m_fbos[0] && m_fbos[0]->isValid()) ? m_fbos[0].get() : nullptr;
    const auto* overlayFbo = (m_fbo && m_fbo->isValid()) ? m_fbo.get() : nullptr;

    auto computeAspectFit = [&](const QOpenGLFramebufferObject* fbo, float& outScaleX, float& outScaleY)
    {
      if(!fbo)
        return;

      outScaleX = 1.0f;
      outScaleY = 1.0f;

      const auto windowAspectRatio = static_cast<float>(this->width()) / static_cast<float>(this->height());
      const auto imageAspectRatio = static_cast<float>(fbo->size().width()) / static_cast<float>(fbo->size().height());
      if(imageAspectRatio > windowAspectRatio)
        outScaleY = windowAspectRatio / imageAspectRatio;
      else
        outScaleX = imageAspectRatio / windowAspectRatio;
    };

    // Base and overlay can have different sizes; preserve each aspect ratio.
    computeAspectFit(baseFbo, baseScaleX, baseScaleY);
    computeAspectFit(overlayFbo, overlayScaleX, overlayScaleY);

    m_quadProgram->bind();
    m_quadVao.bind();

    // Common uniforms
    m_quadProgram->setUniformValue("u_tex", 0);

    // Draw base (no luma key, fully opaque, no blend)
    if(baseFbo)
    {
      glDisable(GL_BLEND);

      QMatrix4x4 mvp;
      // Preserve previous "move" direction (old code used texcoord offsets).
      mvp.translate(static_cast<float>(-xPos), static_cast<float>(-yPos), 0.0f);
      mvp.scale(static_cast<float>(scale_size), static_cast<float>(scale_size), 1.0f);
      mvp.scale(baseScaleX, baseScaleY, 1.0f);

      m_quadProgram->setUniformValue("u_mvp", mvp);
      m_quadProgram->setUniformValue("u_color", QVector4D{1.0f, 1.0f, 1.0f, 1.0f});
      m_quadProgram->setUniformValue("u_brightness", 1.0f);
      m_quadProgram->setUniformValue("u_useLumaKey", false);
      m_quadProgram->setUniformValue("u_lumaThreshold255", 0.0f);
      m_quadProgram->setUniformValue("u_lumaFeather255", 0.0f);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, baseFbo->texture());
      glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // Draw overlay (alpha blend + luminance key)
    if(overlayFbo)
    {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      QMatrix4x4 mvp;
      mvp.translate(static_cast<float>(-overlay_xPos), static_cast<float>(-overlay_yPos), 0.0f);
      mvp.rotate(static_cast<float>(m_rotationRadians * 180.0 / std::numbers::pi), 0.0f, 0.0f, 1.0f);
      mvp.scale(static_cast<float>(overlay_scale_size), static_cast<float>(overlay_scale_size), 1.0f);
      mvp.scale(overlayScaleX, overlayScaleY, 1.0f);

      m_quadProgram->setUniformValue("u_mvp", mvp);
      m_quadProgram->setUniformValue("u_color", QVector4D{m_overlayTint.x(), m_overlayTint.y(), m_overlayTint.z(), static_cast<float>(m_opacity)});
      m_quadProgram->setUniformValue("u_brightness", static_cast<float>(m_overlayBrightness));
      m_quadProgram->setUniformValue("u_useLumaKey", true);
      m_quadProgram->setUniformValue("u_lumaThreshold255", static_cast<float>(m_overlayLuminanceThreshold));
      m_quadProgram->setUniformValue("u_lumaFeather255", static_cast<float>(m_overlayLuminanceFeather));

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, overlayFbo->texture());
      glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    m_quadVao.release();
    m_quadProgram->release();
  }

  auto ImageViewer::RotateImage(std::string&& imagePath) -> void {
      m_rotationRadians += 0.25;
      if (m_rotationRadians > 2 * std::numbers::pi)
        m_rotationRadians -= 2 * std::numbers::pi;
      this->update();
  }

  auto ImageViewer::CleanUp() -> void
  {
    if (m_texture)
    {
      auto textureId = m_texture->textureId();
      glDeleteTextures(1, &textureId);
      m_texture.reset();
    }
  }

  auto ImageViewer::OnNextButtonPressed() -> void 
  {
    if(m_fbos.size() < 2)
      return;

    m_logger.Log(AtlasLogger::LogLevel::Info, "Selecting next base image.");

    // Performs a left rotation on the vector; the base image is always m_fbos[0].
    std::ranges::rotate(m_fbos, m_fbos.begin() + 1);
    this->update();
  }

  auto ImageViewer::OnPrevButtonPressed() -> void 
  {
    if(m_fbos.size() < 2)
      return;

    m_logger.Log(AtlasLogger::LogLevel::Info, "Selecting previous base image.");

    // Performs a right rotation on the vector; the base image is always m_fbos[0].
    std::ranges::rotate(m_fbos, m_fbos.end() - 1);
    this->update();
  }

  auto ImageViewer::OnSliderUpdated(double value) -> void {
      m_logger.Log(AtlasLogger::LogLevel::Info, "Slider updated! New value: {}", value);
      m_opacity = value;
      this->update();
  }

  auto ImageViewer::ResetImage() -> void {
      m_logger.Log(AtlasLogger::LogLevel::Info, "Image reset! We are in the backend.");
      m_rotationRadians = 0.0;
      this->update();
  }

  auto ImageViewer::SaveImage() -> void {
      m_logger.Log(AtlasLogger::LogLevel::Info, "Image saved! We are in the backend.");

      // Get what is displayed or something
      QImage image = this->grabFramebuffer();

      // Pull up the save dialog and let them do their thing
      QString filePath = QFileDialog::getSaveFileName(
              nullptr,
              "Save Image",
              "",                           // Default directory or file name
              "PNG Image (*.png);;JPEG (*.jpg);;BMP Image (*.bmp);;TIF Image (*.tif)"
      );

      image.save(filePath);
  }

  auto ImageViewer::MoveOverlayLeft() -> void {
    m_logger.Log(AtlasLogger::LogLevel::Info, "Image moved to the left! We are in the backend.");
    overlay_xPos += 0.01f;
    this->update();
  }

  auto ImageViewer::MoveOverlayRight() -> void {
    m_logger.Log(AtlasLogger::LogLevel::Info, "Image moved to the right! We are in the backend.");
    overlay_xPos -= 0.01f;
    this->update();
  }

  auto ImageViewer::MoveOverlayUp() -> void {
    m_logger.Log(AtlasLogger::LogLevel::Info, "Image moved up! We are in the backend.");
    overlay_yPos -= 0.01f;
    this->update();
  }

  auto ImageViewer::MoveOverlayDown() -> void {
    m_logger.Log(AtlasLogger::LogLevel::Info, "Image moved down! We are in the backend.");
    overlay_yPos += 0.01f;
    this->update();
  }

  auto ImageViewer::MoveImageDown() -> void {
    m_logger.Log(AtlasLogger::LogLevel::Info, "Image moved down! We are in the backend.");
    yPos += 0.01f;
    this->update();
  }

  auto ImageViewer::MoveImageUp() -> void {
    m_logger.Log(AtlasLogger::LogLevel::Info, "Image moved up! We are in the backend.");
    yPos -= 0.01f;
    this->update();
  }

  auto ImageViewer::MoveImageLeft() -> void {
    m_logger.Log(AtlasLogger::LogLevel::Info, "Image moved left! We are in the backend.");
    xPos += 0.01f;
    this->update();
  }

  auto ImageViewer::MoveImageRight() -> void {
    m_logger.Log(AtlasLogger::LogLevel::Info, "Image moved right! We are in the backend.");
    xPos -= 0.01f;
    this->update();
  }


  auto ImageViewer::ScaleImageUp() -> void {
    m_logger.Log(AtlasLogger::LogLevel::Info, "Image scaled up! We are in the backend.");
    scale_size += 0.2f;
    this->update();
  }

  auto ImageViewer::ScaleImageDown() -> void {
    m_logger.Log(AtlasLogger::LogLevel::Info, "Image scaled down! We are in the backend.");
    scale_size -= 0.2f;
    this->update();
  }

  auto ImageViewer::ScaleOverlayUp() -> void {
    m_logger.Log(AtlasLogger::LogLevel::Info, "Overlay scaled up! We are in the backend.");
    overlay_scale_size += 0.2f;
    this->update();
  }

  auto ImageViewer::ScaleOverlayDown() -> void {
    m_logger.Log(AtlasLogger::LogLevel::Info, "Overlay scaled down! We are in the backend.");
    overlay_scale_size -= 0.2f;
    this->update();
  }

  auto ImageViewer::keyPressEvent(QKeyEvent* event) -> void {
      switch (event->key()) {
        case Qt::Key_Left:
          qDebug() << "Left arrow pressed";
          MoveOverlayLeft();
          break;
        case Qt::Key_Right:
          qDebug() << "Right arrow pressed";
          MoveOverlayRight();
          break;
        case Qt::Key_Up:
          qDebug() << "Up arrow pressed";
          MoveOverlayUp();
          break;
        case Qt::Key_Down:
          qDebug() << "Down arrow pressed";
          MoveOverlayDown();
          break;
        case Qt::Key_R:
          qDebug() << "R Button pressed";
          ScaleImageUp();
          break;
        case Qt::Key_F:
          qDebug() << "F Button pressed";
          ScaleImageDown();
          break;
        case Qt::Key_T:
          qDebug() << "T Button pressed";
          ScaleOverlayUp();
          break;
        case Qt::Key_G:
          qDebug() << "G Button pressed";
          ScaleOverlayDown();
          break;
        case Qt::Key_W:
          qDebug() << "W Button pressed";
          MoveImageUp();
          break;
        case Qt::Key_A:
          qDebug() << "A Button pressed";
          MoveImageLeft();
          break;
        case Qt::Key_S:
          qDebug() << "S Button pressed";
          MoveImageDown();
          break;
        case Qt::Key_D:
          qDebug() << "D Button pressed";
          MoveImageRight();
          break;
        default:
          QOpenGLWindow::keyPressEvent(event);
      }
  }

}