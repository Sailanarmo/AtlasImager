#include "atlasmainwindow_layout.hpp"

#include "atlascontrolwidget.hpp"
#include "AtlasImageViewer/atlasimageviewer.hpp"
#include "AtlasMessenger/atlasmessenger.hpp"

#include <QToolBar>
#include <QWidget>

namespace AtlasGUI
{
  AtlasMainWindowLayout::AtlasMainWindowLayout(QWidget* parent) : QHBoxLayout{parent}
  {
    Initialize();
  }

  auto AtlasMainWindowLayout::Initialize() -> void
  {
    m_atlasToolBar = new QToolBar{};
    m_atlasControlWidget = new AtlasControlWidget{};
    m_atlasimageViewer = new AtlasImageViewer::ImageViewer{};
    m_atlasimageViewerContainer = QWidget::createWindowContainer(m_atlasimageViewer);
    //auto temp = new QOpenGLWindow{};
    m_atlasimageViewerContainer = QWidget::createWindowContainer(m_atlasimageViewer);

    AtlasMessenger::Messenger::Instance().SetImageViewer(m_atlasimageViewer);

    //addWidget(m_atlasToolBar);
    this->addWidget(m_atlasControlWidget, 19);
    this->addWidget(m_atlasimageViewerContainer, 80);
  }

  auto AtlasMainWindowLayout::ResizeLayout() -> void
  {
    //m_atlasToolBar->resize(100, 100);
    m_atlasControlWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
    m_atlasimageViewerContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
  }
}