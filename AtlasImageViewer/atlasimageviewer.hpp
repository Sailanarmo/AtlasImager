#pragma once

#include "AtlasCommon/atlasenums.hpp"

#include <memory>
#include <array>
#include <string_view>

#include <QOpenGLWindow>
#include <QOpenGLTexture>
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>

#include <QVector3D>

class QImage;

/*
* The Atlas Image Viewer is the renderer itself. This will render an image by binding that
* image data to a QOpenGLFramebufferObject. That then can get added to a QOpenGLTexture
* which will display the image in the renderer. 
* 
* TODO: Add a slot to this class that will swap out the images stored in m_fbos when the 
*       best 3 matches are found.
*
*       Using blending, a technique in Graphics Programming, overlay the best matching atlas
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

      auto OnNextButtonPressed() -> void;
      auto OnPrevButtonPressed() -> void;
      auto OnSliderUpdated(double value) -> void;
      auto HandleStateUpdate(const AtlasCommon::AtlasImageViewerState state, const std::string_view imageInformation = "") -> void;
      auto HandleStateUpdate(const AtlasCommon::AtlasImageViewerState state, const std::string_view mainLabelText, const std::string_view progressBarTextformat) -> void;
      auto HandleStateUpdate(const AtlasCommon::AtlasImageViewerState state, const int value) -> void;
      auto RotateImage(std::string&& imagePath) -> void;
      auto ResetImage() -> void;
      auto MoveImageLeft() -> void;
      auto MoveImageRight() -> void;
      auto MoveImageUp() -> void;
      auto MoveImageDown() -> void;
      auto MoveOverlayLeft() -> void;
      auto MoveOverlayRight() -> void;
      auto MoveOverlayUp() -> void;
      auto MoveOverlayDown() -> void;
      auto ScaleImageUp() -> void;
      auto ScaleImageDown() -> void;
      auto ScaleOverlayUp() -> void;
      auto ScaleOverlayDown() -> void;
      auto SaveImage() -> void;

      // Shader/UI driven params (stubs for UI wiring)
      // These are UI-friendly 0..255 inputs; shader normalizes to 0..1.
      auto SetOverlayLuminanceThreshold(double threshold255) -> void;
      auto SetOverlayLuminanceFeather(double feather255) -> void;
      auto SetOverlayBrightness(double brightness) -> void;
      auto SetOverlayTint(const QVector3D& rgb01) -> void;

    protected:
      auto initializeGL() -> void override;
      auto resizeGL(int w, int h) -> void override;
      auto paintGL() -> void override;
      auto keyPressEvent(QKeyEvent* event) -> void override;
    
    signals:
      auto CreateLoadingModelPopupSignal(const std::string_view mainLabelText, const std::string_view progressBarTextformat) -> void;
      auto DisplayLoadingModelPopupSignal() -> void;
      auto DestroyLoadingModelPopupSignal() -> void;
      auto SetMaximumProgressBarValueSignal(const int max) -> void;
      auto UpdateProgressBarValueSignal(const int value) -> void;
    
    private:
      // Framebuffer objects for rendering, takes the 3 best matched images and 
      // sorts them by weight
      //std::array<std::pair<std::unique_ptr<QOpenGLFramebufferObject>, int>, 3> m_fbos;

      std::vector<std::unique_ptr<QOpenGLFramebufferObject>> m_fbos;
      std::unique_ptr<QOpenGLFramebufferObject> m_fbo{nullptr};
      std::unique_ptr<QOffscreenSurface> m_offscreensurface{nullptr};
      std::unique_ptr<QOpenGLContext> m_offscreenContext{nullptr};
      std::unique_ptr<QOpenGLTexture> m_texture{nullptr};
      GLuint m_textureId;
      std::unique_ptr<QOpenGLTexture> overlay_texture{nullptr};
      GLuint overlay_textureId;
      double m_opacity{0.75};
      double m_rotationRadians{0.0};
      double overlay_xPos{0.0};
      double overlay_yPos{0.0};
      double xPos{0.0};
      double yPos{0.0};
      double scale_size{1.0};
      double overlay_scale_size{1.0};

      // Shader pipeline for final screen draw (base + overlay)
      std::unique_ptr<QOpenGLShaderProgram> m_quadProgram{nullptr};
      QOpenGLBuffer m_quadVbo{QOpenGLBuffer::VertexBuffer};
      QOpenGLVertexArrayObject m_quadVao;
      bool m_quadPipelineReady{false};

      // Overlay shader params
      // Stored in 0..255 units to match UI; normalized in shader.
      double m_overlayLuminanceThreshold{13.0};
      double m_overlayLuminanceFeather{5.0};
      double m_overlayBrightness{1.0};
      QVector3D m_overlayTint{1.0f, 1.0f, 1.0f};

      auto CleanUp() -> void;
      auto AddImage(const std::string_view imagePath) -> void;
      auto AddImageWithWeight(const std::string_view imagePath, const double weight) -> void;
      auto ProcessAddImage(const std::string_view imageInformation) -> void;
      auto ProcessAddImageWithWeight(const std::string_view imageInformation) -> void;
      auto RenderMainImage(const std::string_view imagePath) -> void;
      auto CreateImage(const std::string_view imagePath) -> QImage;
      auto CreateTexture(const QImage& image, QOpenGLFunctions* gl_funcs) -> GLuint;
      auto CreateFrameBuffer(const QSize size) -> std::unique_ptr<QOpenGLFramebufferObject>;
      auto DrawToFBO(QOpenGLFramebufferObject* fbo, QOpenGLFunctions* gl_funcs, const GLuint textureId) -> void;
      //auto AddFBOToArray(std::unique_ptr<QOpenGLFramebufferObject>&& fbo, const double weight) -> void;

      auto AddFBOToVector(std::unique_ptr<QOpenGLFramebufferObject>&& fbo) -> void;

      auto EnsureQuadPipeline() -> void;
      
  };
}