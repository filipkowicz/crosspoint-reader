# Kindle Location Sync Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the simplest EPUB reader workflow for manual Kindle location sync.

**Architecture:** Add a small pure Kindle-location mapper, store one per-book Kindle total location count in the existing EPUB cache directory, and wire reader menu actions through the existing keyboard entry activity. The first implementation uses CrossPoint's existing EPUB book/spine size progress mapping for jumps and current-location estimates.

**Tech Stack:** C++20, CrossPoint reader activities, existing `HalStorage` cache APIs, generated I18n strings, CMake/GoogleTest host tests, PlatformIO firmware build.

## Global Constraints

- EPUB only.
- No calibration anchors in the first PR.
- No status bar mode in the first PR.
- Do not change `progress.bin`.
- Store Kindle total locations once per book.
- Label values as Kindle locations, not native CrossPoint locations.
- Keep on-device input simple, using existing `KeyboardEntryActivity`.

---

### Task 1: Pure Kindle Location Mapper

**Files:**
- Create: `src/activities/reader/KindleLocation.h`
- Create: `test/kindle_location/KindleLocationTest.cpp`
- Create: `test/kindle_location/CMakeLists.txt`
- Modify: `test/CMakeLists.txt`

**Interfaces:**
- Produces: `KindleLocation::isValidTotal(uint32_t total) -> bool`
- Produces: `KindleLocation::locationFromProgress(float progress, uint32_t total) -> uint32_t`
- Produces: `KindleLocation::progressFromLocation(uint32_t location, uint32_t total) -> float`
- Produces: `KindleLocation::clampLocation(uint32_t location, uint32_t total) -> uint32_t`

- [ ] **Step 1: Write failing tests**

```cpp
#include <gtest/gtest.h>

#include "KindleLocation.h"

TEST(KindleLocationTest, RejectsMissingTotal) {
  EXPECT_FALSE(KindleLocation::isValidTotal(0));
  EXPECT_EQ(0u, KindleLocation::locationFromProgress(0.5f, 0));
  EXPECT_FLOAT_EQ(0.0f, KindleLocation::progressFromLocation(10, 0));
}

TEST(KindleLocationTest, MapsProgressToOneBasedLocationRange) {
  EXPECT_EQ(1u, KindleLocation::locationFromProgress(0.0f, 8450));
  EXPECT_EQ(8450u, KindleLocation::locationFromProgress(1.0f, 8450));
  EXPECT_EQ(4226u, KindleLocation::locationFromProgress(0.5f, 8450));
}

TEST(KindleLocationTest, ClampsProgressAndLocationInputs) {
  EXPECT_EQ(1u, KindleLocation::locationFromProgress(-1.0f, 100));
  EXPECT_EQ(100u, KindleLocation::locationFromProgress(2.0f, 100));
  EXPECT_EQ(1u, KindleLocation::clampLocation(0, 100));
  EXPECT_EQ(100u, KindleLocation::clampLocation(101, 100));
}

TEST(KindleLocationTest, MapsLocationBackToProgressWithExactEndpoints) {
  EXPECT_FLOAT_EQ(0.0f, KindleLocation::progressFromLocation(1, 8450));
  EXPECT_FLOAT_EQ(1.0f, KindleLocation::progressFromLocation(8450, 8450));
  EXPECT_NEAR(0.5f, KindleLocation::progressFromLocation(4226, 8450), 0.0002f);
}
```

- [ ] **Step 2: Run red test**

Run: `cmake -S test -B build/test -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build/test --target KindleLocationTest`

Expected: fail because `KindleLocation.h` does not exist.

- [ ] **Step 3: Implement mapper**

```cpp
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace KindleLocation {

constexpr uint32_t kMaxTotalLocations = 9999999;

inline bool isValidTotal(uint32_t total) {
  return total > 0 && total <= kMaxTotalLocations;
}

inline uint32_t clampLocation(uint32_t location, uint32_t total) {
  if (!isValidTotal(total)) return 0;
  if (location < 1) return 1;
  if (location > total) return total;
  return location;
}

inline uint32_t locationFromProgress(float progress, uint32_t total) {
  if (!isValidTotal(total)) return 0;
  progress = std::clamp(progress, 0.0f, 1.0f);
  if (total == 1) return 1;
  return static_cast<uint32_t>(std::lround(progress * static_cast<float>(total - 1))) + 1;
}

inline float progressFromLocation(uint32_t location, uint32_t total) {
  if (!isValidTotal(total)) return 0.0f;
  if (total == 1) return 0.0f;
  const uint32_t clamped = clampLocation(location, total);
  return static_cast<float>(clamped - 1) / static_cast<float>(total - 1);
}

}  // namespace KindleLocation
```

- [ ] **Step 4: Run green test**

Run: `cmake --build build/test --target KindleLocationTest && ./build/test/kindle_location/KindleLocationTest`

Expected: pass.

### Task 2: Reader Menu And Keyboard Flow

**Files:**
- Modify: `src/activities/ActivityResult.h`
- Modify: `src/activities/reader/EpubReaderMenuActivity.h`
- Modify: `src/activities/reader/EpubReaderMenuActivity.cpp`
- Modify: `src/activities/reader/EpubReaderActivity.h`
- Modify: `src/activities/reader/EpubReaderActivity.cpp`

**Interfaces:**
- Consumes: `KindleLocation` helper from Task 1.
- Produces: `EpubReaderActivity::loadKindleLocationTotal()`
- Produces: `EpubReaderActivity::saveKindleLocationTotal(uint32_t total)`
- Produces: `EpubReaderActivity::openKindleLocationFlow()`
- Produces: `EpubReaderActivity::jumpToProgress(float progress)`

- [ ] **Step 1: Add menu result type**

Add `struct KindleLocationResult { uint32_t location = 0; };` to `ActivityResult.h` and add it to `ResultVariant`.

- [ ] **Step 2: Add menu action**

Add `KINDLE_LOCATION` to `EpubReaderMenuActivity::MenuAction`, add a row labeled `STR_KINDLE_LOCATION`, pass current and total Kindle locations into the constructor, and show `Loc X/Y` on the right side when configured.

- [ ] **Step 3: Add reader storage helpers**

Read/write `kindle_location.bin` in `epub->getCachePath()` using an 8-byte payload: `K`, `L`, `C`, version `1`, then `uint32_t total` little-endian.

- [ ] **Step 4: Add keyboard flow**

When Kindle total is missing, open `KeyboardEntryActivity` titled `STR_SET_KINDLE_TOTAL`. When present, open `KeyboardEntryActivity` titled `STR_GO_TO_KINDLE_LOCATION` with the current estimated location prefilled. Parse digits with `std::strtoul`, clamp with `KindleLocation`, and jump using `progressFromLocation`.

- [ ] **Step 5: Refactor jumps**

Extract current `jumpToPercent()` body into `jumpToProgress(float progress)`. Keep `jumpToPercent()` as a wrapper that calls `jumpToProgress(percent / 100.0f)`.

### Task 3: Strings And Verification

**Files:**
- Modify: `lib/I18n/translations/english.yaml`
- Generated: `lib/I18n/I18nKeys.h`
- Generated: `lib/I18n/I18nStrings.h`
- Generated: `lib/I18n/I18nStrings.cpp`

**Interfaces:**
- Produces: `STR_KINDLE_LOCATION`
- Produces: `STR_SET_KINDLE_TOTAL`
- Produces: `STR_GO_TO_KINDLE_LOCATION`
- Produces: `STR_KINDLE_LOCATION_FORMAT`

- [ ] **Step 1: Add English strings**

Add:

```yaml
STR_KINDLE_LOCATION: "Kindle location"
STR_SET_KINDLE_TOTAL: "Set Kindle total"
STR_GO_TO_KINDLE_LOCATION: "Go to Kindle location"
STR_KINDLE_LOCATION_FORMAT: "Loc %u/%u"
```

- [ ] **Step 2: Regenerate i18n files**

Run: `python3 scripts/gen_i18n.py lib/I18n/translations lib/I18n/`

Expected: generated I18n files are updated and missing translations fall back to English.

- [ ] **Step 3: Build and test**

Run: `cmake --build build/test && ctest --test-dir build/test --output-on-failure -j`

Expected: host tests pass.

Run: `python3 -m platformio run -e default`

Expected: firmware build passes.
