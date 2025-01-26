#pragma once

#include <QWidget>

namespace AtlasGUI
{
  class AtlasControlWidget : public QWidget
  {
    Q_OBJECT

  public:
    AtlasControlWidget(QWidget* parent = nullptr);
    ~AtlasControlWidget() = default;

  private:
    auto Initialize() -> void;
  };
}