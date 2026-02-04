#include "atlasprogresspopupwidget.hpp"

namespace AtlasGUI
{
  AtlasProgressPopupWidget::AtlasProgressPopupWidget(const QString& mainLoadingText, const QString& format, QWidget* parent) : QWidget{parent, Qt::Window | Qt::FramelessWindowHint}
  {
    m_layout = new AtlasProgressPopupLayout{mainLoadingText, format, this};
    this->setLayout(m_layout);
    this->setFixedSize(400, 200);
    //this->setAttribute(Qt::WA_TranslucentBackground, true);
  }

  auto AtlasProgressPopupWidget::SetMaxProgressBarValue(const int max) -> void
  {
    m_layout->SetMaxProgressBarValue(max);
  }

  auto AtlasProgressPopupWidget::UpdateProgressValue(int value) -> void
  {
    m_layout->SetProgressValue(value);
  }
}