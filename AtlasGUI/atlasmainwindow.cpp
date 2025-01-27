#include "atlasmainwindow.hpp"

#include "atlasmainwindow_layout.hpp"
#include "AtlasCommon/atlasenums.hpp"
#include "AtlasMessenger/atlasmessenger.hpp"

#include <QCloseEvent>

#include <print>

namespace AtlasGUI
{
  AtlasMainWindow::AtlasMainWindow(QWidget* parent) : QFrame{parent}
  {
    Initialize();
  }

  auto AtlasMainWindow::closeEvent(QCloseEvent* event) -> void
  {
    std::println("AtlasMainWindow Closing");
    AtlasMessenger::Messenger::Instance().SendMessage("Shutdown,Shutdown", AtlasCommon::AtlasClasses::AtlasModel);
    event->accept();
  }

  auto AtlasMainWindow::Initialize() -> void
  {
    auto layout = new AtlasMainWindowLayout{};
    this->setLayout(layout);
    this->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
  }
}