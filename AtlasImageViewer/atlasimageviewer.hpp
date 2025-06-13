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

/*
* The Atlas Image Viewer is the renderer itself. This will rander an image by binding that
* image data to a QOpenGLFramebufferObject. That then can get added to a QOpenGLTexsture
* which will display the image in the renderer. 
* 
* TODO: Add a slot to this class that will swap out the images stored in m_fbos when the 
*       best 3 matches are found.
*
*       Using blending, a technique in Graphics Progamming, overlay the best matching atlas
*       model image over the XRay image. Or, find a way to overlay the best matching atlas
*       model image over the XRay image using a 3D Volume (like a cube) where the front of
*       the cube (The Z-Axis [Towards the Monitor]) is the atlas model image and the back of
*       the cube (The -Z-Axis [Going into the Monitor]) is the XRay image.   
*/
namespace AtlasImageViewer
{
  class ImageViewer : public QOpenGLWindow, protected QOpenGLFunctions
  {
    Q_OBJECT
    public:
      ImageViewer();
      ~ImageViewer();

      auto AddImage(std::string&& imagePath, const double weight) -> void;
      auto OnNextButtonPressed() -> void;
      auto OnPrevButtonPressed() -> void;
      auto OnSliderUpdated(int value) -> void;
      auto HandleMessage(const char* message) -> void;

    protected:
      auto initializeGL() -> void override;
      auto resizeGL(int w, int h) -> void override;
      auto paintGL() -> void override;
    
    private:
      // Framebuffer objects for rendering, takes the 3 best matched images and 
      // sorts them by weight
      std::array<std::pair<std::unique_ptr<QOpenGLFramebufferObject>, int>, 3> m_fbos;
      std::unique_ptr<QOpenGLFramebufferObject> m_fbo{nullptr};
      std::unique_ptr<QOffscreenSurface> m_offscreensurface{nullptr};
      std::unique_ptr<QOpenGLTexture> m_texture{nullptr};
      GLuint m_textureId;
      int sliderValue = 0;

      auto CleanUp() -> void;
      auto LoadImage(const std::string_view imagePath) -> void;
      auto CreateImage(const std::string_view imagePath) -> QImage;
      auto CreateNewContext() -> std::unique_ptr<QOpenGLContext>;
      auto CreateTexture(const QImage& image, QOpenGLFunctions* gl_funcs) -> GLuint;
      auto CreateFrameBuffer(const QSize size) -> std::unique_ptr<QOpenGLFramebufferObject>;
      auto DrawToFBO(QOpenGLFramebufferObject* fbo, QOpenGLFunctions* gl_funcs, const GLuint textureId) -> void;
      auto AddFBOToArray(std::unique_ptr<QOpenGLFramebufferObject>&& fbo, const double weight) -> void;
      
  };
}