#pragma once

#include <QVBoxLayout>

class QWidget;

namespace AtlasGUI
{
  class AtlasControlWidgetLayout : public QVBoxLayout
  {
    Q_OBJECT

  public:
    AtlasControlWidgetLayout(QWidget* parent = nullptr);
    ~AtlasControlWidgetLayout() = default;
  
  public slots:
    auto LoadModel() -> void;
    //auto RenderImage() -> void;
    //auto NextImage() -> void;
    //auto PreviousImage() -> void;

  private:
    QWidget* m_ratModelWidget{nullptr};
    QWidget* m_imagePathWidget{nullptr};
    QWidget* m_renderingOptionsWidget{nullptr};
    QWidget* m_imageNavigationWidget{nullptr};

    bool m_isModelLoaded{false};

    auto Initialize() -> void;
    auto BuildRatModelWidget() -> void;
    auto BuildImagePathWidget() -> void;
    auto BuildRenderingOptionsWidget() -> void;
    auto BuildImageNavigationWidget() -> void;

  };
}