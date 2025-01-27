#include "AtlasGUI/atlasmainwindow.hpp"

#include "AtlasModel/model.hpp"
#include "AtlasMessenger/atlasmessenger.hpp"

#include <QApplication>

#include <thread>
#include <print>

namespace 
{
  std::atomic<bool> shut_down{false};
}

auto LaunchQApp(int argc, char** argv) -> int
{
  QApplication app{argc, argv};

  AtlasGUI::AtlasMainWindow mainWindow{};
  //auto model = std::make_shared<AtlasModel::Model>();
  //AtlasMessenger::Messenger::Instance().SetModel(model.get());
  mainWindow.resize(800, 600);
  mainWindow.show();

  QObject::connect(&app, &QApplication::aboutToQuit, []()
  {
    std::println("AtlasMainWindow Closing");
    shut_down = true;
  });

  return app.exec();
}

auto LaunchModelApp() -> void
{
  std::println("Launching Model App");
  auto model = std::make_unique<AtlasModel::Model>(shut_down);
  AtlasMessenger::Messenger::Instance().SetModel(model.get());
  while(!shut_down)
  {
    //std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  std::println("Model App Shutting Down");
  model.reset();
}

auto main(int argc, char** argv) -> int
{
  std::jthread AtlasImager(LaunchQApp, std::ref(argc), std::ref(argv));
  AtlasImager.detach();
  std::jthread ModelApp(LaunchModelApp);
  //QApplication app{argc, argv};

  //AtlasGUI::AtlasMainWindow mainWindow{};
  //auto model = std::make_shared<AtlasModel::Model>();
  //AtlasMessenger::Messenger::Instance().SetModel(model.get());
  //mainWindow.resize(800, 600);
  //mainWindow.show();

  return EXIT_SUCCESS;
}