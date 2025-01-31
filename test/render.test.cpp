#include "modules/engine/render.h"
#include <flecs.h>
#include <gtest/gtest.h>

void systemApplyParentTransform(Transform &t, const Transform *parent);

class RenderModuleTests : public testing::Test {
protected:
  flecs::world world;

public:
  //   RenderModuleTests() { world.import <RenderModule>(); }
};

TEST_F(RenderModuleTests, TransformWithoutParentAppliesRelativeToWorld) {
  // Given
  Point original{10, 20};
  Transform t{original, {}};

  // When
  systemApplyParentTransform(t, nullptr);

  // Then
  EXPECT_EQ(t.relativePosition.x, original.x);
  EXPECT_EQ(t.relativePosition.y, original.y);
  EXPECT_EQ(t.worldPosition.x, original.x);
  EXPECT_EQ(t.worldPosition.y, original.y);
}

TEST_F(RenderModuleTests, TransformWithParentAppliesRelativeToParentWorld) {
  // Given
  Point original{10, 20};
  Transform t{original, {}};
  Transform p{original, original};

  // When
  systemApplyParentTransform(t, &p);

  // Then
  EXPECT_EQ(t.relativePosition.x, original.x);
  EXPECT_EQ(t.relativePosition.y, original.y);
  EXPECT_EQ(t.worldPosition.x, original.x * 2);
  EXPECT_EQ(t.worldPosition.y, original.y * 2);
}
