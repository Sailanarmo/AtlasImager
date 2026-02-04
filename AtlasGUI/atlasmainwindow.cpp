#include "atlasmainwindow.hpp"

#include "AtlasLogger/atlaslogger.hpp"
#include "atlasmainwindow_layout.hpp"

namespace AtlasGUI
{

  static AtlasLogger::Logger m_logger{std::filesystem::current_path().string() + "/Logs/AtlasMainWindow.log", "AtlasGUI::AtlasMainWindow"};

  AtlasMainWindow::AtlasMainWindow(QWidget* parent) : QFrame{parent}
  {
    Initialize();
  }

  auto AtlasMainWindow::Initialize() -> void
  {
    auto layout = new AtlasMainWindowLayout{};
    this->setLayout(layout);
    this->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    QObject::connect(layout, &AtlasMainWindowLayout::CreateLoadingPopupSignal, this, &AtlasMainWindow::CreateLoadingPopup);
    QObject::connect(layout, &AtlasMainWindowLayout::SetMaximumProgressBarValueSignal, this, &AtlasMainWindow::SetMaximumProgressBarValue);
    QObject::connect(layout, &AtlasMainWindowLayout::UpdateProgressBarValueSignal, this, &AtlasMainWindow::UpdateProgressBarValue);
    QObject::connect(layout, &AtlasMainWindowLayout::DisplayLoadingPopupSignal, this, &AtlasMainWindow::DisplayLoadingPopup);
    QObject::connect(layout, &AtlasMainWindowLayout::DestroyLoadingPopupSignal, this, &AtlasMainWindow::DestroyLoadingPopup);
  }

  auto AtlasMainWindow::CreateLoadingPopup(const QString& mainLoadingText, const QString& format) -> void
  {
    m_logger.Log(AtlasLogger::LogLevel::Info, "Created loading popup with main text: {} and format: {}", mainLoadingText.toStdString(), format.toStdString());
    m_loadingPopup = new AtlasProgressPopupWidget{mainLoadingText,format,this};
  }

  auto AtlasMainWindow::DisplayLoadingPopup() -> void
  {
    if(m_loadingPopup)
    {
      m_logger.Log(AtlasLogger::LogLevel::Info, "Displaying loading popup");
      m_loadingPopup->show();
    }
    else
    {
      m_logger.Log(AtlasLogger::LogLevel::Warning, "Attempted to display loading popup but it is null");
    }
  }

  auto AtlasMainWindow::DestroyLoadingPopup() -> void
  {
    if(m_loadingPopup)
    {
      m_loadingPopup->close();
      m_loadingPopup->deleteLater();
      m_loadingPopup = nullptr;
    }
  }

  auto AtlasMainWindow::SetMaximumProgressBarValue(const int max) -> void
  {
    if(m_loadingPopup)
    {
      m_loadingPopup->SetMaxProgressBarValue(max);
      m_logger.Log(AtlasLogger::LogLevel::Info, "Set maximum progress bar value to {}", max);
    }
    else
    {
      m_logger.Log(AtlasLogger::LogLevel::Warning, "Attempted to set progress bar value but loading popup is null");
    }
  }

  auto AtlasMainWindow::UpdateProgressBarValue(const int value) -> void
  {
    if(m_loadingPopup)
    {
      m_loadingPopup->UpdateProgressValue(value);
      m_logger.Log(AtlasLogger::LogLevel::Info, "Updated progress bar value to {}", value);
    }
    else
    {
      m_logger.Log(AtlasLogger::LogLevel::Warning, "Attempted to update progress bar value but loading popup is null");
    }
  }
}