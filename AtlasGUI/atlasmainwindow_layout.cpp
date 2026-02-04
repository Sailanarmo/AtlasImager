#include "atlasmainwindow_layout.hpp"

#include "AtlasLogger/atlaslogger.hpp"
#include "atlascontrolwidget.hpp"
#include "AtlasImageViewer/atlasimageviewer.hpp"
#include "AtlasMessenger/atlasmessenger.hpp"

#include <QToolBar>
#include <QWidget>

namespace AtlasGUI
{

  static AtlasLogger::Logger m_logger{std::filesystem::current_path().string() + "/Logs/AtlasMainWindowLayout.log", "AtlasGUI::AtlasMainWindowLayout"};

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

    AtlasMessenger::Messenger::Instance().SetImageViewer(m_atlasimageViewer);

    QObject::connect(m_atlasimageViewer, &AtlasImageViewer::ImageViewer::CreateLoadingModelPopupSignal, this, &AtlasMainWindowLayout::CreateLoadingModelPopup);
    QObject::connect(m_atlasimageViewer, &AtlasImageViewer::ImageViewer::SetMaximumProgressBarValueSignal, this, &AtlasMainWindowLayout::SetMaximumProgressBarValue);
    QObject::connect(m_atlasimageViewer, &AtlasImageViewer::ImageViewer::DisplayLoadingModelPopupSignal, this, &AtlasMainWindowLayout::DisplayLoadingModelPopup);
    QObject::connect(m_atlasimageViewer, &AtlasImageViewer::ImageViewer::DestroyLoadingModelPopupSignal, this, &AtlasMainWindowLayout::DestroyLoadingModelPopup);

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

  auto AtlasMainWindowLayout::CreateLoadingModelPopup(const AtlasCommon::AtlasDataSet dataSet) -> void
  {
    m_logger.Log(AtlasLogger::LogLevel::Info, "Creating loading model popup for dataset: {}", AtlasCommon::DataSetToString(dataSet));
    std::string datasetName;
    switch(dataSet)
    {
      case AtlasCommon::AtlasDataSet::LGN:
        datasetName = "LGN";
        break;
      case AtlasCommon::AtlasDataSet::PAG:
        datasetName = "PAG";
        break;
      default:
        datasetName = "Unknown Dataset";
        break;
    }

    auto mainLoadingText = QString::fromStdString("Loading " + datasetName + " Model...");
    auto format = QString::fromStdString("Loading " + datasetName + " Images: %p%");

    m_logger.Log(AtlasLogger::LogLevel::Info, "Emitting signal to create loading popup with main text: '{}' and format: '{}'", mainLoadingText.toStdString(), format.toStdString());

    emit CreateLoadingPopupSignal(mainLoadingText, format);
  }

  auto AtlasMainWindowLayout::SetMaximumProgressBarValue(const int max) -> void
  {
    m_logger.Log(AtlasLogger::LogLevel::Info, "Emitting signal to set maximum progress bar value: {}", max);
    emit SetMaximumProgressBarValueSignal(max);
  }

  auto AtlasMainWindowLayout::DisplayLoadingModelPopup() -> void
  {
    m_logger.Log(AtlasLogger::LogLevel::Info, "Emitting signal to display loading popup");
    emit DisplayLoadingPopupSignal();
  }

  auto AtlasMainWindowLayout::DestroyLoadingModelPopup() -> void
  {
    m_logger.Log(AtlasLogger::LogLevel::Info, "Emitting signal to destroy loading popup");
    emit DestroyLoadingPopupSignal();
  }
}