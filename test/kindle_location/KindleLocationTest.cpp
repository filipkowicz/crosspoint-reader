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
