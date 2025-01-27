#pragma once

#include <QFrame>

class QCloseEvent;

namespace AtlasGUI
{
  class AtlasMainWindow : public QFrame
  {
    Q_OBJECT

  public:
    AtlasMainWindow(QWidget* parent = nullptr);
    ~AtlasMainWindow() = default;

  protected:
    auto closeEvent(QCloseEvent* event) -> void override;

  private:
    auto Initialize() -> void;
  };
}