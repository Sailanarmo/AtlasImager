#include "atlasfilelogger.hpp"

#include <fstream>

namespace AtlasLogger
{
  FileLogger::FileLogger(const std::string_view logFilePath, const std::string_view loggerClassName)
    : m_logFilePath(logFilePath), m_loggerClassName(loggerClassName)
  {
    this->initializeLogFile();
  }

  FileLogger::~FileLogger()
  {
    std::ofstream ofs(m_logFilePath, std::ios::app);
    ofs << "--- Atlas Logger Instance for " << m_loggerClassName << " Destroyed ---\n";
    ofs.close();
  }

  auto FileLogger::initializeLogFile() -> void
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

  auto FileLogger::Write(
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

    ofs << "[" << std::chrono::current_zone()->to_local(timestamp) << "] "
        << "[" << LogLevelToString(level) << "] "
        << "[" << m_loggerClassName << "] "
        << message << "\n";

    ofs.close();
  }

} // namespace AtlasLogger
