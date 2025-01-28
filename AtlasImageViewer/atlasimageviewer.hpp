#pragma once

#include <memory>
#include <array>
#include <string_view>

#include <QOpenGLWindow>
#include <QOpenGLTexture>
#include <QOpenGLFunctions>
#include <QOffscreenSurface>
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

      auto AddImage(std::string&& imagePath, const double weight) -> void;
      auto HandleMessage(const char* message) -> void;

    protected:
      auto initializeGL() -> void override;
      auto resizeGL(int w, int h) -> void override;
      auto paintGL() -> void override;
    
    private:
      // Framebuffer objects for rendering, takes the 5 best matched images and 
      // sorts them by weight
      std::array<std::pair<std::unique_ptr<QOpenGLFramebufferObject>, int>, 3> m_fbos;
      std::unique_ptr<QOpenGLFramebufferObject> m_fbo{nullptr};
      std::unique_ptr<QOffscreenSurface> m_offscreensurface{nullptr};
      std::unique_ptr<QOpenGLTexture> m_texture{nullptr};
      GLuint m_textureId;

      auto CleanUp() -> void;
      auto LoadImage(const std::string_view imagePath) -> void;
      auto CreateImage(const std::string_view imagePath) -> QImage;
      auto CreateNewContext() -> std::unique_ptr<QOpenGLContext>;
      auto CreateTexture(const QImage& image, QOpenGLFunctions* gl_funcs) -> GLuint;
      auto CreateFrameBuffer(const QSize size) -> std::unique_ptr<QOpenGLFramebufferObject>;
      auto AddFBOToArray(std::unique_ptr<QOpenGLFramebufferObject>&& fbo, const double weight) -> void;
      
  };
}