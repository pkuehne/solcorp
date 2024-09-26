#include "phase.h"

phase::phase(flecs::world &world) {
  // Register phases
  auto preFramePhase = world.entity("PreFrame").add(flecs::Phase);
  auto validatePhase =
      world.entity("Validate").add(flecs::Phase).depends_on(preFramePhase);
  auto PostValidatePhase =
      world.entity("PostValidate").add(flecs::Phase).depends_on(validatePhase);
  auto UpdatePhase =
      world.entity("Update").add(flecs::Phase).depends_on(PostValidatePhase);
  auto guiPhase = world.entity("Gui").add(flecs::Phase).depends_on(UpdatePhase);

  auto preRenderPhase =
      world.entity("PreRender").add(flecs::Phase).depends_on(guiPhase);
  auto renderPhase =
      world.entity("Render").add(flecs::Phase).depends_on(preRenderPhase);
  // auto postRenderPhase =
  world.entity("PostRender").add(flecs::Phase).depends_on(renderPhase);
}
