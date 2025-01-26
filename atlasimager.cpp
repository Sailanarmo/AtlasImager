#include "AtlasGUI/atlasmainwindow.hpp"

#include "AtlasModel/model.hpp"
#include "AtlasMessenger/atlasmessenger.hpp"

#include <QApplication>

auto main(int argc, char** argv) -> int
{
  QApplication app{argc, argv};

  AtlasGUI::AtlasMainWindow mainWindow{};
  auto model = std::make_shared<AtlasModel::Model>();
  AtlasMessenger::Messenger::Instance().SetModel(model.get());
  mainWindow.resize(800, 600);
  mainWindow.show();

  return app.exec();
}