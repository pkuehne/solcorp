#include "modules/stats/stats.h"
#include <gtest/gtest.h>

class StatTest : public ::testing::Test {
protected:
  Stat stat = Stat("test_stat", 10.0);
};

TEST_F(StatTest, TestBase) {
  // Given
  // When
  // Then
  EXPECT_DOUBLE_EQ(stat.base(), 10.0);
}

TEST_F(StatTest, TestValue) {
  // Given
  EXPECT_DOUBLE_EQ(stat.value(), 10.0);
  Modifier mod = {"test_stat", 5.0, 2.0};

  // When
  stat.addModifier(mod);

  // Then
  EXPECT_DOUBLE_EQ(stat.value(), 30.0);
}

TEST_F(StatTest, TestReset) {
  // Given
  Modifier mod = {"test_stat", 5.0, 2.0};
  stat.addModifier(mod);

  // When
  stat.reset();

  // Then
  EXPECT_DOUBLE_EQ(stat.value(), 10.0);
}

TEST_F(StatTest, TestAddModifier) {
  // Given
  Modifier mod = {"test_stat", 5.0, 2.0};

  // When
  bool result = stat.addModifier(mod);

  // Then
  EXPECT_TRUE(result);
  EXPECT_DOUBLE_EQ(stat.value(), 30.0);
}

TEST_F(StatTest, TestAddUnrelatedModifier) {
  // Given
  Modifier wrong_mod = {"wrong_stat", 5.0, 2.0};

  // When
  bool result = stat.addModifier(wrong_mod);

  // Then
  EXPECT_FALSE(result);
  EXPECT_DOUBLE_EQ(stat.value(), 10.0);
}
