#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace KindleLocation {

constexpr uint32_t kMaxTotalLocations = 9999999;

inline bool isValidTotal(const uint32_t total) { return total > 0 && total <= kMaxTotalLocations; }

inline uint32_t clampLocation(const uint32_t location, const uint32_t total) {
  if (!isValidTotal(total)) {
    return 0;
  }
  if (location < 1) {
    return 1;
  }
  if (location > total) {
    return total;
  }
  return location;
}

inline uint32_t locationFromProgress(float progress, const uint32_t total) {
  if (!isValidTotal(total)) {
    return 0;
  }
  progress = std::clamp(progress, 0.0f, 1.0f);
  if (total == 1) {
    return 1;
  }
  return static_cast<uint32_t>(std::lround(progress * static_cast<float>(total - 1))) + 1;
}

inline float progressFromLocation(const uint32_t location, const uint32_t total) {
  if (!isValidTotal(total)) {
    return 0.0f;
  }
  if (total == 1) {
    return 0.0f;
  }
  const uint32_t clamped = clampLocation(location, total);
  return static_cast<float>(clamped - 1) / static_cast<float>(total - 1);
}

}  // namespace KindleLocation
