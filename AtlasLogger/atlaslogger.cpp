#include "atlaslogger.hpp"

#include <fstream>

namespace AtlasLogger
{
  Logger::Logger(const std::string_view logFilePath, const std::string_view loggerClassName)
    : m_logFilePath(logFilePath), m_loggerClassName(loggerClassName)
  {
    this->initializeLogFile();
  }

  auto Logger::initializeLogFile() -> void
  {
    if(!std::filesystem::exists(m_logFilePath.parent_path()))
    {
      std::filesystem::create_directories(m_logFilePath.parent_path());
    }

    if(!std::filesystem::exists(m_logFilePath))
    {
      std::ofstream ofs(m_logFilePath, std::ios::app);
      ofs << "--- Atlas Logger Initialized for " << m_loggerClassName << " ---\n";
      ofs.close();
    }
    else
    {
      std::ofstream ofs(m_logFilePath, std::ios::app);
      ofs << "--- New Instance of Atlas Logger Initialized for " << m_loggerClassName << " ---\n";
      ofs.close();
    }
  }

  auto Logger::LogMessageToDisk(
    const LogLevel level,
    const std::chrono::system_clock::time_point& timestamp,
    const std::string_view message
  ) -> void
  {
    std::ofstream ofs(m_logFilePath, std::ios::app);
    if(!ofs.is_open())
    {
      return;
    }

    const auto timeT = std::chrono::system_clock::to_time_t(timestamp);

  #ifdef __EMSCRIPTEN__
    ofs << "[" << std::ctime(&timeT) << "] "
  #else
    ofs << "[" << std::chrono::current_zone()->to_local(timestamp) << "] "
  #endif
        << "[" << LogLevelToString(level) << "] "
        << "[" << m_loggerClassName << "] "
        << message << "\n";
        //<< " (at " << loc.file_name() << ":" << loc.line() << " in " << loc.function_name() << ")\n";

    ofs.close();
  }
  
}