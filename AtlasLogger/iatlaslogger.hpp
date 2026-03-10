#pragma once

#include <format>
#include <cstdint>
#include <chrono>
#include <source_location>
#include <string>

namespace AtlasLogger
{
  enum class LogLevel : std::uint8_t
  {
    Debug,
    Info,
    Warning,
    Error,
    Critical
  };

  inline const auto LogLevelToString = [](const LogLevel level) -> std::string_view
  {
    switch(level)
    {
      case LogLevel::Debug:    return "DEBUG";
      case LogLevel::Info:     return "INFO";
      case LogLevel::Warning:  return "WARNING";
      case LogLevel::Error:    return "ERROR";
      case LogLevel::Critical: return "CRITICAL";
      default:                 return "UNKNOWN";
    }
  };

  template<class... Args>
  struct format_string_with_source
  {
    std::format_string<Args...> fmt;
    std::source_location loc;

    template<class T>
    requires std::convertible_to<T, std::string_view>
    consteval format_string_with_source(
      const T& fmt,
      std::source_location loc = std::source_location::current()
    ) : fmt(fmt), loc(loc) {}
  };

  class IAtlasLogger
  {
  public:
    virtual ~IAtlasLogger() = default;

    template<typename... Args>
    auto Log(const LogLevel level,
             format_string_with_source<std::type_identity_t<Args>...> fmtWithLoc,
             Args&&... args) -> void
    {
      const auto timestamp = std::chrono::system_clock::now();
      const auto formattedMessage = std::format(
        "{}:{} {}",
        fmtWithLoc.loc.file_name(),
        fmtWithLoc.loc.line(),
        std::vformat(fmtWithLoc.fmt.get(), std::make_format_args(args...))
      );
      this->Write(level, timestamp, formattedMessage);
    }

  protected:
    virtual auto Write(
      LogLevel level,
      const std::chrono::system_clock::time_point& timestamp,
      std::string_view message
    ) -> void = 0;
  };

} // namespace AtlasLogger
