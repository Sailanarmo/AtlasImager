#include "atlasmainwindow.hpp"

#include "atlasmainwindow_layout.hpp"

namespace AtlasGUI
{
  AtlasMainWindow::AtlasMainWindow(QWidget* parent) : QFrame{parent}
  {
    Initialize();
  }

  auto AtlasMainWindow::Initialize() -> void
  {
    auto layout = new AtlasMainWindowLayout{};
    this->setLayout(layout);
    this->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
  }
}