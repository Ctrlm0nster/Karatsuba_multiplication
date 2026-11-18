#pragma once

#include <string_view>

#define KARATSUBA_VERSION_MAJOR 1
#define KARATSUBA_VERSION_MINOR 0
#define KARATSUBA_VERSION_PATCH 0
#define KARATSUBA_VERSION_STRING "1.0.0"

namespace km {

inline constexpr int versionMajor = KARATSUBA_VERSION_MAJOR;
inline constexpr int versionMinor = KARATSUBA_VERSION_MINOR;
inline constexpr int versionPatch = KARATSUBA_VERSION_PATCH;
inline constexpr std::string_view versionString = KARATSUBA_VERSION_STRING;

}  // namespace km

namespace karatsuba_multiplication = km;
