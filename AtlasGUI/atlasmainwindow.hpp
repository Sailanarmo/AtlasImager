#pragma once

#include <QFrame>

namespace AtlasGUI
{
  class AtlasMainWindow : public QFrame
  {
    Q_OBJECT

  public:
    AtlasMainWindow(QWidget* parent = nullptr);
    ~AtlasMainWindow() = default;

  private:
    auto Initialize() -> void;
  };
}