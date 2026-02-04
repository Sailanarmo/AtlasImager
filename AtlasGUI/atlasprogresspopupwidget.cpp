#include "atlasprogresspopupwidget.hpp"

#include <QPainter>

namespace AtlasGUI
{
  AtlasProgressPopupWidget::AtlasProgressPopupWidget(const QString& mainLoadingText, const QString& format, QWidget* parent) : QWidget{parent, Qt::Window | Qt::FramelessWindowHint}
  {
    this->setFixedSize(400, 200);
    this->setAttribute(Qt::WA_TranslucentBackground);

    m_frameWidget = new QFrame{this};
    m_layout = new AtlasProgressPopupLayout{mainLoadingText, format, m_frameWidget};

    m_frameWidget->setLayout(m_layout);
    m_frameWidget->setFixedSize(this->size().width() - 20, this->size().height() - 20);
    m_frameWidget->setStyleSheet("QFrame {"
      "    background-color: lightgray;"
      "    border: 2px solid black;"
      "    border-radius: 12px;" // Adjust the radius as needed
      "}"
    );

    auto popupWidgetLayout = new QVBoxLayout{this};
    popupWidgetLayout->addWidget(m_frameWidget);

    this->setLayout(popupWidgetLayout);
  }

  auto AtlasProgressPopupWidget::SetMaxProgressBarValue(const int max) -> void
  {
    m_layout->SetMaxProgressBarValue(max);
  }

  auto AtlasProgressPopupWidget::UpdateProgressValue(int value) -> void
  {
    m_layout->SetProgressValue(value);
  }

  auto AtlasProgressPopupWidget::paintEvent(QPaintEvent* event) -> void
  {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QBrush(Qt::darkGray));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(this->rect(), 10, 10);
    QWidget::paintEvent(event);
  }
}