#pragma once

#include "AtlasCommon/atlasenums.hpp"

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
  
  public slots:
    auto CreateLoadingModelPopup(const AtlasCommon::AtlasDataSet dataSet) -> void;
    auto SetMaximumProgressBarValue(const int max) -> void;
    auto DisplayLoadingModelPopup() -> void;
    auto DestroyLoadingModelPopup() -> void;
  
  signals:
    auto CreateLoadingPopupSignal(const QString& mainLoadingText, const QString& format) -> void;
    auto SetMaximumProgressBarValueSignal(const int max) -> void;
    auto DisplayLoadingPopupSignal() -> void;
    auto DestroyLoadingPopupSignal() -> void;

  private:

    QToolBar* m_atlasToolBar{nullptr};
    QWidget* m_atlasControlWidget{nullptr}; 
    QWidget* m_atlasimageViewerContainer{nullptr};
    AtlasImageViewer::ImageViewer* m_atlasimageViewer{nullptr};

    auto Initialize() -> void;
    auto ResizeLayout() -> void;
  };
}