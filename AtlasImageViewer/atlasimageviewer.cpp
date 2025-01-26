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
    //CleanUp();
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

  // should this be a unique_ptr of an OpenCV Mat passed in by rvalue?
  // or should this be an array of images already sorted? 
  auto ImageViewer::AddImage(const unsigned char* data, const int weight) -> void
  {
    auto minWeight = std::ranges::min_element(m_fbos, [](const auto& lhs, const auto& rhs)
    {
      return lhs.second < rhs.second;
    });

    if (weight > minWeight->second)
    {
      auto minWeightIndex = std::distance(m_fbos.begin(), minWeight);
      m_fbos[minWeightIndex].second = weight;
      m_fbos[minWeightIndex].first = std::make_unique<QOpenGLFramebufferObject>(QSize{128, 128});
    }
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
        //AddImage(argument);
      }
    }
  }

  auto ImageViewer::initializeGL() -> void
  {
    initializeOpenGLFunctions();
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
}