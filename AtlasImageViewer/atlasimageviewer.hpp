#pragma once

#include <memory>
#include <array>
#include <string_view>

#include <QOpenGLWindow>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QOpenGLFramebufferObject>

class QImage;

namespace AtlasImageViewer
{
  class ImageViewer : public QOpenGLWindow, protected QOpenGLFunctions
  {
    Q_OBJECT
    public:
      ImageViewer();
      ~ImageViewer();

      auto AddImage(const unsigned char* data, const int weight) -> void;
      auto HandleMessage(const char* message) -> void;

    protected:
      auto initializeGL() -> void override;
      auto resizeGL(int w, int h) -> void override;
      auto paintGL() -> void override;
    
    private:
      // Framebuffer objects for rendering, takes the 5 best matched images and 
      // sorts them by weight
      std::array<std::pair<std::unique_ptr<QOpenGLFramebufferObject>, int>, 5> m_fbos;
      std::unique_ptr<QOpenGLFramebufferObject> m_fbo{nullptr};
      std::unique_ptr<QOpenGLTexture> m_texture{nullptr};
      GLuint m_textureId;

      auto LoadImage(const std::string_view imagePath) -> void;
      auto CleanUp() -> void;
      
  };
}