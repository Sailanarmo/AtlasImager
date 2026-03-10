#pragma once

#include "iatlaslogger.hpp"

#include <filesystem>
#include <chrono>
#include <format>
#include <string>

namespace AtlasLogger
{
  // Builds a YYYY-MM-DD date string using the local timezone.
  // Used by call sites to construct date-stamped log file paths.
  // Native builds only — do not include this header in WASM translation units.
  inline const auto GetCurrentDateString = []() -> std::string
  {
    const auto& tz = std::chrono::current_zone();
    const auto localTime = std::chrono::zoned_time{tz, std::chrono::system_clock::now()};
    return std::format("{0:%F}", localTime);
  };

  class FileLogger : public IAtlasLogger
  {
  public:
    FileLogger(std::string_view logFilePath, std::string_view loggerClassName);
    ~FileLogger() override;

  protected:
    auto Write(
      LogLevel level,
      const std::chrono::system_clock::time_point& timestamp,
      std::string_view message
    ) -> void override;

  private:
    auto initializeLogFile() -> void;

    const std::filesystem::path m_logFilePath;
    const std::string           m_loggerClassName;
  };

} // namespace AtlasLogger
