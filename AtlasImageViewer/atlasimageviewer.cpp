#include "atlasimageviewer.hpp"

#include "AtlasMessenger/atlasmessenger.hpp"

#include <ranges>
#include <algorithm>
#include <print>

#include <QImage>

namespace AtlasImageViewer
{
  ImageViewer::ImageViewer() : m_textureId{0}
  {
    std::ranges::fill(m_fbos, std::pair{nullptr, 0});
  }

  ImageViewer::~ImageViewer()
  {
    CleanUp();
    m_offscreensurface.reset();
    
    std::ranges::for_each(m_fbos, [](auto& fbo){
      if(fbo.first)
        fbo.first.reset();
    });

  }

  auto ImageViewer::LoadImage(const std::string_view imagePath) -> void
  {
    std::println("ImageViewer::LoadImage: imagePath: {}", imagePath);

    if(m_texture)
      CleanUp();

    auto image = QImage{imagePath.data()}; 

    auto glImage = image.convertToFormat(QImage::Format_RGBA8888);

    m_texture = std::make_unique<QOpenGLTexture>(glImage.mirrored());
    m_texture->setMinificationFilter(QOpenGLTexture::Linear);
    m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
    m_texture->setWrapMode(QOpenGLTexture::ClampToEdge);

    this->update();
  }

  auto ImageViewer::CreateImage(const std::string_view imagePath) -> QImage
  {
    auto image = QImage(imagePath.data());
    auto glImage = image.convertToFormat(QImage::Format_RGBA8888);
    return glImage;
  }

  auto ImageViewer::CreateNewContext() -> std::unique_ptr<QOpenGLContext>
  {
    auto newContext = std::make_unique<QOpenGLContext>();
    newContext->setFormat(this->context()->format());
    newContext->setShareContext(this->context());
    newContext->create();
    return newContext;
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

  auto ImageViewer::AddFBOToArray(std::unique_ptr<QOpenGLFramebufferObject>&& fbo, const double weight) -> void
  {
    if(m_fbos[0].first == nullptr)
    {
      m_fbos[0].first = std::move(fbo);
      m_fbos[0].second = weight;
    }
    else
    {
      auto minWeight = std::ranges::min_element(m_fbos, [](const auto& lhs, const auto& rhs)
      {
        return lhs.second < rhs.second;
      });

      if (weight > minWeight->second)
      {
        auto minWeightIndex = std::distance(m_fbos.begin(), minWeight);
        m_fbos[minWeightIndex].second = weight;
        m_fbos[minWeightIndex].first = std::move(fbo);
      }
    }
    std::println("Successfully added FBO to array");
  }

  // should this be a unique_ptr of an OpenCV Mat passed in by rvalue?
  // or should this be an array of images already sorted? 
  auto ImageViewer::AddImage(std::string&& imagePath, const double weight) -> void
  {
    const auto curImagePath = std::move(imagePath);

    std::println("Creating QImage");
    auto image = CreateImage(curImagePath);
    std::println("Creating a Context");
    auto tempContext = std::move(CreateNewContext());

    tempContext->makeCurrent(m_offscreensurface.get());

    auto gl_funcs = tempContext->functions();
    std::println("Creating a new texture");
    auto textureId = CreateTexture(image, gl_funcs);

    auto fbo = CreateFrameBuffer(image.size());

    if(!fbo->isValid())
    {
      fbo.reset();
      gl_funcs->glDeleteTextures(1, &textureId);
    }

    fbo->bind();
    gl_funcs->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
    fbo->release();

    std::println("Adding FBO to Array.");
    AddFBOToArray(std::move(fbo), weight);

  }

  auto ImageViewer::HandleMessage(const char* message) -> void
  {
    auto msg = std::string{message};

    auto commaPos = msg.find(',');
    if (commaPos != std::string::npos)
    {
      auto command = msg.substr(0, commaPos);
      auto argument = msg.substr(commaPos + 1);

      if (command == "RenderImage")
      {
        //RenderImage(argument);
      }
      else if (command == "LoadImage")
      {
        std::println("ImageViewer::HandleMessage: LoadImage");
        std::println("ImageViewer::HandleMessage: argument: {}", argument);
        LoadImage(argument);
      }
      else if (command == "AddImage")
      {
        std::println("ImageViewer::HandleMessage:: AddImage");
        std::println("ImageViewer::HandleMessage: argument: {}", argument);
        auto colonPos = argument.find(':');
        if(colonPos != std::string::npos)
        {
          auto imagePath = argument.substr(0, colonPos);
          auto weight = std::stod(argument.substr(colonPos + 1));

          std::println("ImageViewer::HandleMessage: adding the image {} with weight: {}", imagePath, weight);
          AddImage(std::move(imagePath),weight);
          //TODO: Blend and render the images.
        }
      }
      else if (command == "NextButton") {
          // std::println("Next Button was Pressed! We are in the backend.");
          OnNextButtonPressed();
      }
      else if (command == "PrevButton") {
          OnPrevButtonPressed();
      }
    }
  }

  auto ImageViewer::initializeGL() -> void
  {
    initializeOpenGLFunctions();
    m_offscreensurface = std::make_unique<QOffscreenSurface>();
    m_offscreensurface->setFormat(this->context()->format());
    m_offscreensurface->create();
  }

  auto ImageViewer::resizeGL(int w, int h) -> void
  {
    glViewport(0, 0, w, h);
  }

  auto ImageViewer::paintGL() -> void
  {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_texture) 
    {
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, m_texture->textureId());

      auto windowAspectRatio = static_cast<float>(this->width()) / static_cast<float>(this->height());
      auto imageAspectRatio = static_cast<float>(m_texture->width()) / static_cast<float>(m_texture->height());

      auto scaleX = 1.0f;
      auto scaleY = 1.0f;

      if(imageAspectRatio > windowAspectRatio)
        scaleY = windowAspectRatio / imageAspectRatio;
      else
        scaleX = imageAspectRatio / windowAspectRatio;

      // Render a textured quad
      glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-scaleX, -scaleY);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(scaleX, -scaleY);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(scaleX, scaleY);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-scaleX, scaleY);
      glEnd();

      glBindTexture(GL_TEXTURE_2D, 0);
      glDisable(GL_TEXTURE_2D);
    }
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

  auto ImageViewer::OnNextButtonPressed() -> void {
      std::println("Next Button was Pressed! We are in the backend.");
//      std::swap(m_fbo, m_fbos[0].first);
//      std::ranges::rotate(m_fbos, m_fbos.begin() + 1);
//      this->update();
  }

  auto ImageViewer::OnPrevButtonPressed() -> void {
      std::println("Prev Button was Pressed! We are in the backend.");
//      std::swap(m_fbo, m_fbos[0].first);
//      std::ranges::rotate(m_fbos, m_fbos.begin() - 1);
//      this->update();
  }
}