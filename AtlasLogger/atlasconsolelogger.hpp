#pragma once

#include "iatlaslogger.hpp"

#include <emscripten.h>
#include <string>

namespace AtlasLogger
{
  // Browser console logger for Emscripten builds.
  // Accepts the same two-argument constructor signature as FileLogger so that
  // all call sites compile unchanged — the file path argument is ignored.
  class ConsoleLogger : public IAtlasLogger
  {
  public:
    ConsoleLogger(std::string_view /*logFilePath*/, std::string_view loggerClassName)
      : m_loggerClassName(loggerClassName) {}

    ~ConsoleLogger() override = default;

  protected:
    auto Write(
      LogLevel level,
      const std::chrono::system_clock::time_point& /*timestamp*/,
      std::string_view message
    ) -> void override
    {
      const int flags =
        (level == LogLevel::Error || level == LogLevel::Critical) ? EM_LOG_ERROR :
        (level == LogLevel::Warning)                              ? EM_LOG_WARN  :
                                                                    EM_LOG_CONSOLE;
      emscripten_log(flags, "[%s] %s",
                     m_loggerClassName.c_str(),
                     std::string{message}.c_str());
    }

  private:
    const std::string m_loggerClassName;
  };

} // namespace AtlasLogger
