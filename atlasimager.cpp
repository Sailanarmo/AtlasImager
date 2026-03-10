#include "AtlasGUI/atlasmainwindow.hpp"

#include "AtlasModel/model.hpp"
#include "AtlasLogger/atlaslogger.hpp"
#include "AtlasMessenger/atlasmessenger.hpp"

#include <QMainWindow>
#include <QApplication>

#include <thread>

namespace 
{
  std::atomic<bool> shut_down{false};
}

auto LaunchModelApp(AtlasLogger::Logger& logger) -> void
{
  logger.Log(AtlasLogger::LogLevel::Info, "Launching Model App");
  auto model = std::make_unique<AtlasModel::Model>();
  AtlasMessenger::Messenger::Instance().SetModel(model.get());
  while(!shut_down)
  {
  }
  logger.Log(AtlasLogger::LogLevel::Info, "Model App Shutting Down");
  model.reset();
}

auto main(int argc, char** argv) -> int
{

  QApplication app{argc, argv};
  QCoreApplication::setApplicationName("Atlas Imager");

  auto m_logger = AtlasLogger::Logger{
    QStandardPaths::standardLocations(QStandardPaths::AppConfigLocation).at(1).toStdString() + 
    "/Atlas-Imager/Logs/" + AtlasLogger::GetCurrentDateString() + "/Main.log", "Main"
  };

  QObject::connect(&app, &QApplication::aboutToQuit, [&m_logger]()
  {
    m_logger.Log(AtlasLogger::LogLevel::Info, "AtlasMainWindow Closing");
    shut_down = true;
  });

  std::thread ModelApp(LaunchModelApp, std::ref(m_logger));
  ModelApp.detach();

  auto mainWindow = QMainWindow{};
  auto mainApp = new AtlasGUI::AtlasMainWindow{};
  mainWindow.setCentralWidget(mainApp);
  mainWindow.resize(800, 600);
  mainWindow.setWindowTitle("Atlas Imager");
  mainWindow.show();

  return app.exec();
}