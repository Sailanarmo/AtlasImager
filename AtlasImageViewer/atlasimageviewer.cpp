#include "atlasimageviewer.hpp"

#include "AtlasLogger/atlaslogger.hpp"
#include "AtlasMessenger/atlasmessenger.hpp"


#include <ranges>
#include <print>
#include <numbers>
#include <expected>
#include <charconv>
#include <algorithm>
#include <optional>
#include <cmath>

#include <QImage>
#include <QKeyEvent>
#include <QFileDialog>

#include <QColor>

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
    constexpr auto kPi = std::numbers::pi;

    auto DegreesToRadians(const double degrees) -> double
    {
      return degrees * (kPi / 180.0);
    }

    auto NormalizeRadiansSigned(double radians) -> double
    {
      // Normalize to [-pi, pi]
      radians = std::fmod(radians, 2.0 * kPi);
      if(radians > kPi)
        radians -= 2.0 * kPi;
      else if(radians < -kPi)
        radians += 2.0 * kPi;
      return radians;
    }

    auto ParseDouble(std::string_view text) -> std::optional<double>
    {
      double value = 0.0;
      const auto* begin = text.data();
      const auto* end = text.data() + text.size();
      if(std::from_chars(begin, end, value).ec == std::errc{})
        return value;
      return std::nullopt;
    }

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

    // Your cell images have black backgrounds, so do a simple near-black key.
    // Tune threshold/feather if you see dark cell pixels getting cut out.
    ApplyBlackBackgroundAlpha(glImage, /*threshold=*/12, /*feather=*/24);

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


  auto ImageViewer::initializeGL() -> void
  {
    initializeOpenGLFunctions();
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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    // If the viewport isn't square, rotating in raw NDC space visually shears/stretches.
    // Apply an aspect correction when rotating vertices so the quad rotates rigidly.
    const float viewportAspect = (this->height() > 0)
      ? static_cast<float>(this->width()) / static_cast<float>(this->height())
      : 1.0f;

    // Before drawing FBOs
    auto baseScaleX = 1.0f;
    auto baseScaleY = 1.0f;
    auto overlayScaleX = 1.0f;
    auto overlayScaleY = 1.0f;
    const auto* overlayFbo = (!m_fbos.empty() && m_fbos[0] && m_fbos[0]->isValid()) ? m_fbos[0].get() : nullptr;
    const auto* baseFbo = (m_fbo && m_fbo->isValid()) ? m_fbo.get() : nullptr;

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

    // Render a textured quad
    auto rotate = [&](float x, float y, float& outX, float& outY) {
      const float cosA = std::cos(static_cast<float>(m_rotationRadians));
      const float sinA = std::sin(static_cast<float>(m_rotationRadians));

      // Scale X into a square-space, rotate, then un-scale back.
      const float xSq = x * viewportAspect;
      const float ySq = y;
      const float xr = cosA * xSq - sinA * ySq;
      const float yr = sinA * xSq + cosA * ySq;
      outX = xr / viewportAspect;
      outY = yr;
    };

    float x0 = -overlayScaleX, y0 = -overlayScaleY;
    float x1 =  overlayScaleX, y1 = -overlayScaleY;
    float x2 =  overlayScaleX, y2 =  overlayScaleY;
    float x3 = -overlayScaleX, y3 =  overlayScaleY;

    float rx0, ry0, rx1, ry1, rx2, ry2, rx3, ry3;
    rotate(x0, y0, rx0, ry0);
    rotate(x1, y1, rx1, ry1);
    rotate(x2, y2, rx2, ry2);
    rotate(x3, y3, rx3, ry3);


    // Draw Base FBO
    /*
    if(!m_fbos.empty() && m_fbos[0].first && m_fbos[0].first->isValid()) {
      glPushMatrix();
      glBindTexture(GL_TEXTURE_2D, m_fbos[0].first->texture());
      glColor4f(1.0, 1.0, 1.0,1.0);
      glScalef(scale_size, scale_size, 1.0f);
      glBegin(GL_QUADS);
      glTexCoord2f(0.0f + xPos, 0.0f + yPos); glVertex2f(-scaleX, -scaleY);
      glTexCoord2f(1.0f + xPos, 0.0f + yPos); glVertex2f(scaleX, -scaleY);
      glTexCoord2f(1.0f + xPos, 1.0f + yPos); glVertex2f(scaleX, scaleY);
      glTexCoord2f(0.0f + xPos, 1.0f + yPos); glVertex2f(-scaleX, scaleY);
      glEnd();
      glPopMatrix();
    }
    */

    if(baseFbo) {
      glPushMatrix();
      glBindTexture(GL_TEXTURE_2D, baseFbo->texture());
      glColor4f(1.0, 1.0, 1.0, 1.0);

      glBegin(GL_QUADS);
      glTexCoord2f(0.0f + xPos, 0.0f + yPos); glVertex2f(-baseScaleX, -baseScaleY);
      glTexCoord2f(1.0f + xPos, 0.0f + yPos); glVertex2f(baseScaleX, -baseScaleY);
      glTexCoord2f(1.0f + xPos, 1.0f + yPos); glVertex2f(baseScaleX, baseScaleY);
      glTexCoord2f(0.0f + xPos, 1.0f + yPos); glVertex2f(-baseScaleX, baseScaleY);
      glEnd();
      glPopMatrix();
    }

    // Draw Overlay fbo
    if (overlayFbo)
    {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      glPushMatrix();
      glBindTexture(GL_TEXTURE_2D, overlayFbo->texture());
      glScalef(overlay_scale_size, overlay_scale_size, 1.0f);
      glColor4f(1.0, 1.0, 1.0, m_opacity);
      glScalef(scale_size, scale_size, 1.0f);

      glBegin(GL_QUADS);
      glTexCoord2f(0.0f + overlay_xPos, 0.0f + overlay_yPos); glVertex2f(rx0, ry0);
      glTexCoord2f(1.0f + overlay_xPos, 0.0f + overlay_yPos); glVertex2f(rx1, ry1);
      glTexCoord2f(1.0f + overlay_xPos, 1.0f + overlay_yPos); glVertex2f(rx2, ry2);
      glTexCoord2f(0.0f + overlay_xPos, 1.0f + overlay_yPos); glVertex2f(rx3, ry3);
      glEnd();
      glPopMatrix();
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
  }

  auto ImageViewer::RotateImage(const double angleDegrees) -> void
  {
    // GUI slider provides degrees in [-180, 180]. Internally we store radians.
    m_rotationRadians = NormalizeRadiansSigned(DegreesToRadians(angleDegrees));
    this->update();
  }

  auto ImageViewer::ScaleImage(const int value) -> void
  {
    // GUI slider provides integer percent in [1, 300]. Map to [0.01, 3.0].
    const int clamped = std::clamp(value, 1, 300);
    scale_size = static_cast<double>(clamped) / 100.0;
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
        if(const auto deg = ParseDouble(imageInformation))
          this->RotateImage(*deg);
        else
          m_logger.Log(AtlasLogger::LogLevel::Error, "Failed to parse rotation degrees from: {}", imageInformation);
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
      case AtlasCommon::AtlasImageViewerState::RotateImage:
        m_logger.Log(AtlasLogger::LogLevel::Info, "Rotate Image requested");
        this->RotateImage(static_cast<double>(value) / 1000.0);
        break;
      case AtlasCommon::AtlasImageViewerState::ScaleImage:
        m_logger.Log(AtlasLogger::LogLevel::Info, "Scale Image requested");
        this->ScaleImage(value);
        break;
      default:
        break;
    }
  }

}