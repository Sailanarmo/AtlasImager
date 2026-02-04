#include "atlasprogresspopup_layout.hpp"

namespace AtlasGUI
{
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

    this->addWidget(m_mainLoadingLabel);
    this->addWidget(m_progressBar);
  }

  auto AtlasProgressPopupLayout::SetMainLoadingText(const QString& text) -> void
  {
    m_mainLoadingLabel->setText(text);
  }

  auto AtlasProgressPopupLayout::SetMaxProgressBarValue(const int max) -> void
  {
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(max);
  }

  auto AtlasProgressPopupLayout::SetProgressBarText(const QString& text) -> void
  {
    m_progressBar->setFormat(text);
  }

  auto AtlasProgressPopupLayout::SetProgressValue(int value) -> void
  {
    m_progressBar->setValue(value);
  }

}