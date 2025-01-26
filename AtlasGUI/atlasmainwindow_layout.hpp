#pragma once

#include <QHBoxLayout>

namespace AtlasImageViewer
{
  class ImageViewer;
}

class QWidget;
class QToolBar;

namespace AtlasGUI
{
  class AtlasMainWindowLayout : public QHBoxLayout
  {
    Q_OBJECT

  public:
    AtlasMainWindowLayout(QWidget* parent = nullptr);
    ~AtlasMainWindowLayout() = default;

  private:

    QToolBar* m_atlasToolBar{nullptr};
    QWidget* m_atlasControlWidget{nullptr}; 
    QWidget* m_atlasimageViewerContainer{nullptr};
    AtlasImageViewer::ImageViewer* m_atlasimageViewer{nullptr};

    auto Initialize() -> void;
    auto ResizeLayout() -> void;
  };
}