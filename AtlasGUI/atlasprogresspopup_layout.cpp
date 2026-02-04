#include "atlasprogresspopup_layout.hpp"

#include "AtlasLogger/atlaslogger.hpp"

namespace AtlasGUI
{

  static AtlasLogger::Logger m_logger{std::filesystem::current_path().string() + "/Logs/AtlasProgressPopupLayout.log", "AtlasGUI::AtlasProgressPopupLayout"};

  AtlasProgressPopupLayout::AtlasProgressPopupLayout(const QString& mainLoadingText, const QString& format, QWidget* parent) : QVBoxLayout{parent}
  {
    this->Initialize();
    this->SetMainLoadingText(mainLoadingText);
    this->SetProgressBarText(format);
  }

  auto AtlasProgressPopupLayout::Initialize() -> void
  {
    m_progressBar = new QProgressBar{};
    m_mainLoadingLabel = new QLabel{};

    m_progressBar->setValue(0);
    this->addWidget(m_mainLoadingLabel);
    this->addWidget(m_progressBar);
  }

  auto AtlasProgressPopupLayout::SetMainLoadingText(const QString& text) -> void
  {
    m_mainLoadingLabel->setText(text);
  }

  auto AtlasProgressPopupLayout::SetMaxProgressBarValue(const int max) -> void
  {
    m_logger.Log(AtlasLogger::LogLevel::Info, "Setting max progress bar value to {}", max);
    m_progressBar->setRange(0, max);
    m_progressBar->setValue(0);
  }

  auto AtlasProgressPopupLayout::SetProgressBarText(const QString& text) -> void
  {
    m_progressBar->setFormat(text);
  }

  auto AtlasProgressPopupLayout::SetProgressValue(int value) -> void
  {
    m_logger.Log(AtlasLogger::LogLevel::Info, "Setting progress value to {}", value);
    m_progressBar->setValue(value);
  }

}