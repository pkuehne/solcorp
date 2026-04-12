#include "engine.h"
#include "gui.h"
#include "input.h"
#include "modules/engine/movement.h"
#include "render.h"
#include <cstddef>

EngineModule::EngineModule(flecs::world &world) {
  // Register components
  world.component<Color>()
      .member("r", &Color::r)
      .member("g", &Color::g)
      .member("b", &Color::b)
      .member("a", &Color::a);
  world.component<Point>().member<float>("x").member<float>("y");
  world.component<Transform>()
      .member("relativePosition", &Transform::relativePosition)
      .member("worldPosition", &Transform::worldPosition);
  world.component<Renderer>()
      .member<uintptr_t>("window", 1, offsetof(Renderer, window))
      .member<uintptr_t>("renderer", 1, offsetof(Renderer, renderer))
      .add(flecs::Singleton);
  world.component<Texture>()
      .member<size_t>("ptr")
      .member("width", &Texture::width)
      .member("height", &Texture::height);
  world.component<SpriteFlip>();
  world.component<Sprite>()
      .member("texture", &Sprite::texture)
      .member("tile", &Sprite::tile)
      .member("x", &Sprite::x)
      .member("y", &Sprite::y)
      .member("width", &Sprite::width)
      .member("height", &Sprite::height)
      .member("rotation", &Sprite::rotation)
      .member("flip", &Sprite::flip);
  world.component<Text>()
      .member("text", &Text::text)
      .member("color", &Text::color)
      .member("rotation", &Text::rotation)
      .member("flip", &Text::flip);
  world.component<Font>()
      .member("name", &Font::name)
      .member("point_size", &Font::point_size)
      .member<size_t>("ptr");
  world.component<Window>().member("open", &Window::open);
  world.component<KeyDown>().member("key", &KeyDown::key).add(flecs::Singleton);
  world.component<KeyUp>().member("key", &KeyUp::key).add(flecs::Singleton);
  world.component<KeyPressed>().add(flecs::Singleton);
  world.component<MouseDown>().add(flecs::Singleton);
  world.component<MouseUp>().add(flecs::Singleton);
  world.component<Velocity>()
      .member("x", &Velocity::x)
      .member("y", &Velocity::y);
  world.component<Expire>().member("millis", &Expire::millis);

  initialiseGraphics(world);

  registerRender(world);
  registerInput(world);
  registerGui(world);
  registerMovement(world);
}
