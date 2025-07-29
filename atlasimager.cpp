#include "AtlasGUI/atlasmainwindow.hpp"

#include "AtlasModel/model.hpp"
#include "AtlasMessenger/atlasmessenger.hpp"

#include <QMainWindow>
#include <QApplication>

#include <thread>
#include <print>

namespace 
{
  std::atomic<bool> shut_down{false};
}

auto LaunchModelApp() -> void
{
  std::println("Launching Model App");
  auto model = std::make_unique<AtlasModel::Model>();
  AtlasMessenger::Messenger::Instance().SetModel(model.get());
  while(!shut_down)
  {
  }
  std::println("Model App Shutting Down");
  model.reset();
}

auto main(int argc, char** argv) -> int
{

  QApplication app{argc, argv};
  QObject::connect(&app, &QApplication::aboutToQuit, []()
  {
    std::println("AtlasMainWindow Closing");
    shut_down = true;
  });

  std::thread ModelApp(LaunchModelApp);
  ModelApp.detach();

  auto mainWindow = QMainWindow{};
  auto mainApp = new AtlasGUI::AtlasMainWindow{};
  mainWindow.setCentralWidget(mainApp);
  mainWindow.resize(800, 600);
  mainWindow.setWindowTitle("Atlas Imager");
  mainWindow.show();

  return app.exec();
}