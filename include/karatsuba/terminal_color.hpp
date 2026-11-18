#pragma once

#include <cstdlib>
#include <string_view>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace km {
namespace color {

inline bool isColorSupported() {
  const char* noColor = std::getenv("NO_COLOR");
  if (noColor && *noColor != '\0') {
    return false;
  }

#ifdef _WIN32
  HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode = 0;
  return GetConsoleMode(output, &mode) &&
         SetConsoleMode(output, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
  const char* term = std::getenv("TERM");
  return isatty(STDOUT_FILENO) && term && std::string_view(term) != "dumb";
#endif
}

inline const char* bold(bool enabled = true) {
  return enabled ? "\033[1m" : "";
}

inline const char* dim(bool enabled = true) {
  return enabled ? "\033[2m" : "";
}

inline const char* red(bool enabled = true) {
  return enabled ? "\033[1;31m" : "";
}

inline const char* green(bool enabled = true) {
  return enabled ? "\033[1;32m" : "";
}

inline const char* yellow(bool enabled = true) {
  return enabled ? "\033[1;33m" : "";
}

inline const char* blue(bool enabled = true) {
  return enabled ? "\033[1;34m" : "";
}

inline const char* magenta(bool enabled = true) {
  return enabled ? "\033[1;35m" : "";
}

inline const char* cyan(bool enabled = true) {
  return enabled ? "\033[1;36m" : "";
}

inline const char* reset(bool enabled = true) {
  return enabled ? "\033[0m" : "";
}

}  // namespace color
}  // namespace km

// Global alias for backwards compatibility
inline bool useColor() {
  return km::color::isColorSupported();
}
