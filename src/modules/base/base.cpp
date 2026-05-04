#include "base.h"
#include <string>

flecs::entity PostStartPhase;
flecs::entity PreFramePhase;
flecs::entity ValidatePhase;
flecs::entity PostValidatePhase;
flecs::entity UpdatePhase;
flecs::entity GuiPhase;
flecs::entity PreRenderPhase;
flecs::entity RenderPhase;
flecs::entity PostRenderPhase;
flecs::entity PostFramePhase;

BaseModule::BaseModule(flecs::world &world) {
  // Register phases
  PostStartPhase =
      world.entity("PostStart").add(flecs::Phase).depends_on(flecs::OnStart);

  PreFramePhase = world.entity("PreFrame").add(flecs::Phase);
  ValidatePhase =
      world.entity("Validate").add(flecs::Phase).depends_on(PreFramePhase);
  PostValidatePhase =
      world.entity("PostValidate").add(flecs::Phase).depends_on(ValidatePhase);
  UpdatePhase =
      world.entity("Update").add(flecs::Phase).depends_on(PostValidatePhase);
  GuiPhase = world.entity("Gui").add(flecs::Phase).depends_on(UpdatePhase);
  PreRenderPhase =
      world.entity("PreRender").add(flecs::Phase).depends_on(GuiPhase);
  RenderPhase =
      world.entity("Render").add(flecs::Phase).depends_on(PreRenderPhase);
  PostRenderPhase =
      world.entity("PostRender").add(flecs::Phase).depends_on(RenderPhase);
  PostFramePhase =
      world.entity("PostFrame").add(flecs::Phase).depends_on(PostRenderPhase);

  world.component<std::string>()
      .opaque(flecs::String) // Opaque type that maps to string
      .serialize([](const flecs::serializer *s, const std::string *data) {
        const char *str = data->c_str();
        return s->value(flecs::String, static_cast<const void *>(&str));
      })
      .assign_string([](std::string *data, const char *value) {
        *data = value; // Assign new value to std::string
      });

  // Game clock
  world.component<Simulation>()
      .member("speed", &Simulation::speed)
      .add(flecs::Singleton);
  world.component<Game>().member("day", &Game::day).add(flecs::Singleton);
  auto simTimer = world.timer("SimTimer").interval(0.5f);
  simTimer.stop();
  world.set<Simulation>(Simulation{simTimer});
  world.set<Game>({});
}
