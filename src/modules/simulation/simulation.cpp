#include "simulation.h"
#include "SDL_keycode.h"
#include "modules/base/base.h"
#include "modules/engine/engine.h"
#include "modules/engine/gui.h"
#include "modules/engine/input.h"
#include "modules/lua/lua.h"
#include "modules/simulation/celestial_browser.h"
#include "modules/simulation/developer_window.h"
#include "spdlog/spdlog.h"

void systemUpdateSimDate(Game &game);
void systemQuitOnEscape(flecs::iter &, size_t, const KeyDown);
void systemShowWindows(flecs::iter &, size_t, const KeyDown);

SimulationModule::SimulationModule(flecs::world &world) {
  world.import <BaseModule>();

  registerEngineComponents(world);

  // Register components
  world.component<CelestialBody>()
      .member("semi_major_axis", &CelestialBody::semi_major_axis)
      .member("eccentricity", &CelestialBody::eccentricity)
      .member("inclination", &CelestialBody::inclination)
      .member("longitude_of_ascending_node",
              &CelestialBody::longitude_of_ascending_node)
      .member("argument_of_periapsis", &CelestialBody::argument_of_periapsis)
      .member("mean_anomaly_at_epoch", &CelestialBody::mean_anomaly_at_epoch)
      .member("retrograde", &CelestialBody::retrograde)
      .member("radius", &CelestialBody::radius)
      .member("gravity", &CelestialBody::gravity)
      .member("density", &CelestialBody::density)
      .member("mass", &CelestialBody::mass)
      .member("gm", &CelestialBody::gm)
      .member("rotation_period", &CelestialBody::rotation_period)
      .member("albedo", &CelestialBody::albedo);
  world.component<Developer>().add(flecs::Singleton);
  world.component<DeveloperWindow>().member(
      "show_metrics_window", &DeveloperWindow::show_metrics_window);
  world.component<CelestialBrowser>().member("selected_body",
                                             &CelestialBrowser::selected_body);
  world.component<TargetOrbit>()
      .member("altitude", &TargetOrbit::altitude)
      .member("inclination", &TargetOrbit::inclination);

  world.component<Company>()
      .member("name", &Company::name)
      .member("balance", &Company::balance)
      .add(flecs::Singleton);

  // Create Singletons
  world.add<Developer>();
  world.add<Company>();

  register_component_lua<Game>(world, "Game", [](LuaFieldBuilder<Game> &b) {
    b.field<&Game::day>("day");
  });

  register_component_lua<Company>(
      world, "Company", [](LuaFieldBuilder<Company> &b) {
        b.field<&Company::name>("name").field<&Company::balance>("balance");
      });

  // Register systems
  auto sim = world.get<Simulation>();
  world.system<Game>("Update Simulation Date")
      .tick_source(sim.speed)
      .kind(UpdatePhase)
      .each(systemUpdateSimDate);

  world.system<const KeyDown>("Quit on Esc")
      .kind(ValidatePhase)
      .each(systemQuitOnEscape);
  world.system<const KeyDown>("Show Windows")
      .kind(ValidatePhase)
      .each(systemShowWindows);
  world.system("Register Windows")
      .kind(flecs::OnStart)
      .immediate()
      .run(systemRegisterWindows);
  world.system("Generate Sol System")
      .kind(flecs::OnStart)
      .immediate()
      .run(systemGenerateSolSystem);
}

void systemUpdateSimDate(Game &game) {
  game.day++;
  spdlog::info("It's Day {}", game.day);
}

void systemQuitOnEscape(flecs::iter &it, size_t, const KeyDown event) {
  if (event.key == SDLK_ESCAPE) {
    it.world().quit();
  }
}

void systemShowWindows(flecs::iter &it, size_t, const KeyDown event) {
  auto world = it.world();
  if (event.key == SDLK_d) {
    showDeveloperWindow(world);
  }
  if (event.key == SDLK_b) {
    showCelestialBrowser(world);
  }
}

void systemRegisterWindows(flecs::iter &it) {
  auto world = it.world();
  registerWindow("Celestial Browser", drawCelestialBrowser, world)
      .set<CelestialBrowser>({});
  registerWindow("Developer Window", drawDeveloperWindow, world)
      .set<DeveloperWindow>({});
}

void systemGenerateSolSystem(flecs::iter &it) {
  spdlog::debug("Generating Sol System...");
  auto world = it.world();

  auto sun = world.entity("Sun").set<CelestialBody>({
      .semi_major_axis = 0.0,
      .eccentricity = 0.0,
      .inclination = 0.0,
      .longitude_of_ascending_node = 0.0,
      .argument_of_periapsis = 0.0,
      .mean_anomaly_at_epoch = 0.0,
      .retrograde = false,
      .radius = 695700000.0,
      .gravity = 274.0,
      .density = 1408.0,
      .mass = 1.9885e30,
      .gm = 1.32712440018e20,
      .rotation_period = 2192832.0,
      .albedo = 0.0,
  });

  world.entity("Mercury")
      .set<CelestialBody>({
          .semi_major_axis = 57909226541.52439,
          .eccentricity = 0.20563593,
          .inclination = 0.12225994793212572,
          .longitude_of_ascending_node = 0.8435309949656006,
          .argument_of_periapsis = 0.5083625818084809,
          .mean_anomaly_at_epoch = 3.050705116248391,
          .retrograde = false,
          .radius = 2439700.0,
          .gravity = 3.7,
          .density = 5427.0,
          .mass = 3.3011e23,
          .gm = 2.2032e13,
          .rotation_period = 5067033.84,
          .albedo = 0.088,
      })
      .child_of(sun);

  world.entity("Venus")
      .set<CelestialBody>({
          .semi_major_axis = 108209474537.37917,
          .eccentricity = 0.00677672,
          .inclination = 0.05924827411109566,
          .longitude_of_ascending_node = 1.3383157224083446,
          .argument_of_periapsis = 0.9585806304888396,
          .mean_anomaly_at_epoch = 0.8792381031921823,
          .retrograde = false,
          .radius = 6051800.0,
          .gravity = 8.87,
          .density = 5243.0,
          .mass = 4.8675e24,
          .gm = 3.24858599e14,
          .rotation_period = 20997360.0,
          .albedo = 0.75,
      })
      .child_of(sun);

  auto earth = world.entity("Earth")
                   .set<CelestialBody>({
                       .semi_major_axis = 149598261150.4425,
                       .eccentricity = 0.01671123,
                       .inclination = -0.00000026720990848033185,
                       .longitude_of_ascending_node = 0.0,
                       .argument_of_periapsis = 1.7966014740491711,
                       .mean_anomaly_at_epoch = -0.04316391697638581,
                       .retrograde = false,
                       .radius = 6371000.0,
                       .gravity = 9.80665,
                       .density = 5514.0,
                       .mass = 5.97237e24,
                       .gm = 3.986004418e14,
                       .rotation_period = 86164.0905,
                       .albedo = 0.306,
                   })
                   .child_of(sun);
  world.entity("Moon")
      .set<CelestialBody>({
          .semi_major_axis = 384400000.0,
          .eccentricity = 0.0549,
          .inclination = 0.08979719001534528,
          .longitude_of_ascending_node = 2.183259703385116,
          .argument_of_periapsis = 3.371273012690141,
          .mean_anomaly_at_epoch = 2.013245780213528,
          .retrograde = false,
          .radius = 1737400.0,
          .gravity = 1.62,
          .density = 3344.0,
          .mass = 7.342e22,
          .gm = 4.902801e12,
          .rotation_period = 2360591.0,
          .albedo = 0.11,
      })
      .child_of(earth);

  world.entity("Low Orbit")
      .set<TargetOrbit>({.altitude = 200'000.0, .inclination = 0.0})
      .child_of(earth);
  world.entity("Polar Orbit")
      .set<TargetOrbit>({.altitude = 200'000.0, .inclination = deg2rad(90.0)})
      .child_of(earth);
  world.entity("Transfer Orbit")
      .set<TargetOrbit>({.altitude = 35'786'000.0, .inclination = 0.0})
      .child_of(earth);
  world.entity("Synchronous Orbit")
      .set<TargetOrbit>({.altitude = 35'786'000.0, .inclination = 0.0})
      .child_of(earth);

  auto mars = world.entity("Mars")
                  .set<CelestialBody>({
                      .semi_major_axis = 227939366344.5385,
                      .eccentricity = 0.0933941,
                      .inclination = 0.03228859115900383,
                      .longitude_of_ascending_node = 0.8649512068711695,
                      .argument_of_periapsis = -1.2877332456216607,
                      .mean_anomaly_at_epoch = 0.3386557814120793,
                      .retrograde = false,
                      .radius = 3389500.0,
                      .gravity = 3.711,
                      .density = 3933.0,
                      .mass = 6.4171e23,
                      .gm = 4.282837e13,
                      .rotation_period = 88642.6632,
                      .albedo = 0.25,
                  })
                  .child_of(sun);
  world.entity("Phobos")
      .set<CelestialBody>({
          .semi_major_axis = 9376000.0,
          .eccentricity = 0.0151,
          .inclination = 0.0,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 11266.7,
          .gravity = 0.0057,
          .density = 1860.0,
          .mass = 1.0659e16,
          .gm = 7.087e5,
          .rotation_period = 27552.0,
          .albedo = 0.07,
      })
      .child_of(mars);

  world.entity("Deimos")
      .set<CelestialBody>({
          .semi_major_axis = 23463000.0,
          .eccentricity = 0.00033,
          .inclination = 0.0,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 6200.0,
          .gravity = 0.003,
          .density = 1470.0,
          .mass = 1.4762e15,
          .gm = 9.615e4,
          .rotation_period = 109080.0,
          .albedo = 0.068,
      })
      .child_of(mars);

  world.entity("Ceres")
      .set<CelestialBody>({
          .semi_major_axis = 414010000000.0,
          .eccentricity = 0.075822,
          .inclination = 0.1849586447958657,
          .longitude_of_ascending_node = 1.333289568682072,
          .argument_of_periapsis = 2.987938064617217,
          .mean_anomaly_at_epoch = 5.98085718540448,
          .retrograde = false,
          .radius = 473000.0,
          .gravity = 0.27,
          .density = 2160.0,
          .mass = 9.393e20,
          .gm = 6.26325e10,
          .rotation_period = 32668.8,
          .albedo = 0.09,
      })
      .child_of(sun);

  world.entity("Pallas")
      .set<CelestialBody>({
          .semi_major_axis = 414970000000.0,
          .eccentricity = 0.23067,
          .inclination = 0.37049525105022886,
          .longitude_of_ascending_node = 1.7002899450667904,
          .argument_of_periapsis = 3.354619380504511,
          .mean_anomaly_at_epoch = 3.053859271012634,
          .retrograde = false,
          .radius = 256000.0,
          .gravity = 0.23,
          .density = 2900.0,
          .mass = 2.11e20,
          .gm = 1.4102e10,
          .rotation_period = 107798.4,
          .albedo = 0.15,
      })
      .child_of(sun);

  world.entity("Vesta")
      .set<CelestialBody>({
          .semi_major_axis = 353260000000.0,
          .eccentricity = 0.08874,
          .inclination = 0.12232045072870662,
          .longitude_of_ascending_node = 1.703305962,
          .argument_of_periapsis = 2.036937137,
          .mean_anomaly_at_epoch = 5.963,
          .retrograde = false,
          .radius = 262700.0,
          .gravity = 0.25,
          .density = 3456.0,
          .mass = 2.59076e20,
          .gm = 1.728e10,
          .rotation_period = 18835.2,
          .albedo = 0.42,
      })
      .child_of(sun);

  auto jupiter = world.entity("Jupiter")
                     .set<CelestialBody>({
                         .semi_major_axis = 778340821344.9,
                         .eccentricity = 0.04838624,
                         .inclination = 0.022767601968366663,
                         .longitude_of_ascending_node = 1.7534314474965515,
                         .argument_of_periapsis = -1.4990269948612492,
                         .mean_anomaly_at_epoch = 0.3429473263260368,
                         .retrograde = false,
                         .radius = 69911000.0,
                         .gravity = 24.79,
                         .density = 1326.0,
                         .mass = 1.8982e27,
                         .gm = 1.26686534e17,
                         .rotation_period = 35730.0,
                         .albedo = 0.343,
                     })
                     .child_of(sun);
  world.entity("Io")
      .set<CelestialBody>({
          .semi_major_axis = 421800000.0,
          .eccentricity = 0.0041,
          .inclination = 0.0,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 1821500.0,
          .gravity = 1.796,
          .density = 3528.0,
          .mass = 8.9319e22,
          .gm = 5.9599e12,
          .rotation_period = 152853.504,
          .albedo = 0.63,
      })
      .child_of(jupiter);

  world.entity("Europa")
      .set<CelestialBody>({
          .semi_major_axis = 671100000.0,
          .eccentricity = 0.009,
          .inclination = 0.0,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 1560800.0,
          .gravity = 1.315,
          .density = 3013.0,
          .mass = 4.7998e22,
          .gm = 3.2027e12,
          .rotation_period = 306822.336,
          .albedo = 0.67,
      })
      .child_of(jupiter);

  world.entity("Ganymede")
      .set<CelestialBody>({
          .semi_major_axis = 1070400000.0,
          .eccentricity = 0.0013,
          .inclination = 0.0,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 2634100.0,
          .gravity = 1.428,
          .density = 1942.0,
          .mass = 1.4819e23,
          .gm = 9.8878e12,
          .rotation_period = 618153.6,
          .albedo = 0.43,
      })
      .child_of(jupiter);

  world.entity("Callisto")
      .set<CelestialBody>({
          .semi_major_axis = 1882700000.0,
          .eccentricity = 0.0074,
          .inclination = 0.0,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 2410300.0,
          .gravity = 1.235,
          .density = 1834.0,
          .mass = 1.0759e23,
          .gm = 7.1793e12,
          .rotation_period = 1441939.2,
          .albedo = 0.22,
      })
      .child_of(jupiter);

  auto saturn = world.entity("Saturn")
                    .set<CelestialBody>({
                        .semi_major_axis = 1426666422006.9587,
                        .eccentricity = 0.05386179,
                        .inclination = 0.04338874330931084,
                        .longitude_of_ascending_node = 1.9837835429754036,
                        .argument_of_periapsis = -0.36762823281234114,
                        .mean_anomaly_at_epoch = -0.744289273004183,
                        .retrograde = false,
                        .radius = 58232000.0,
                        .gravity = 10.44,
                        .density = 687.0,
                        .mass = 5.6834e26,
                        .gm = 3.7931187e16,
                        .rotation_period = 38361.6,
                        .albedo = 0.342,
                    })
                    .child_of(sun);
  world.entity("Mimas")
      .set<CelestialBody>({
          .semi_major_axis = 185520000.0,
          .eccentricity = 0.0196,
          .inclination = 0.0,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 198200.0,
          .gravity = 0.064,
          .density = 1150.0,
          .mass = 3.7493e19,
          .gm = 2.504e9,
          .rotation_period = 81492.0,
          .albedo = 0.6,
      })
      .child_of(saturn);

  world.entity("Enceladus")
      .set<CelestialBody>({
          .semi_major_axis = 238020000.0,
          .eccentricity = 0.0047,
          .inclination = 0.0,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 252100.0,
          .gravity = 0.113,
          .density = 1608.0,
          .mass = 1.08022e20,
          .gm = 7.210e9,
          .rotation_period = 118320.0,
          .albedo = 0.81,
      })
      .child_of(saturn);

  world.entity("Tethys")
      .set<CelestialBody>({
          .semi_major_axis = 294670000.0,
          .eccentricity = 0.0001,
          .inclination = 0.0,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 531100.0,
          .gravity = 0.145,
          .density = 984.0,
          .mass = 6.17449e20,
          .gm = 4.008e10,
          .rotation_period = 190080.0,
          .albedo = 0.8,
      })
      .child_of(saturn);

  world.entity("Dione")
      .set<CelestialBody>({
          .semi_major_axis = 377400000.0,
          .eccentricity = 0.0022,
          .inclination = 0.0,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 561400.0,
          .gravity = 0.232,
          .density = 1480.0,
          .mass = 1.095452e21,
          .gm = 7.311e10,
          .rotation_period = 236160.0,
          .albedo = 0.7,
      })
      .child_of(saturn);

  world.entity("Rhea")
      .set<CelestialBody>({
          .semi_major_axis = 527040000.0,
          .eccentricity = 0.0010,
          .inclination = 0.0,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 763800.0,
          .gravity = 0.264,
          .density = 1230.0,
          .mass = 2.306518e21,
          .gm = 1.539e11,
          .rotation_period = 390960.0,
          .albedo = 0.6,
      })
      .child_of(saturn);

  world.entity("Titan")
      .set<CelestialBody>({
          .semi_major_axis = 1221870000.0,
          .eccentricity = 0.0288,
          .inclination = 0.0,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 2575000.0,
          .gravity = 1.352,
          .density = 1880.0,
          .mass = 1.3452e23,
          .gm = 8.978e12,
          .rotation_period = 1377648.0,
          .albedo = 0.22,
      })
      .child_of(saturn);

  world.entity("Hyperion")
      .set<CelestialBody>({
          .semi_major_axis = 1481100000.0,
          .eccentricity = 0.1042,
          .inclination = 0.0,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 133000.0,
          .gravity = 0.056,
          .density = 544.0,
          .mass = 5.6e18,
          .gm = 3.736e8,
          .rotation_period = 1847520.0,
          .albedo = 0.3,
      })
      .child_of(saturn);

  world.entity("Iapetus")
      .set<CelestialBody>({
          .semi_major_axis = 3561300000.0,
          .eccentricity = 0.0286,
          .inclination = 0.0,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 734500.0,
          .gravity = 0.223,
          .density = 1088.0,
          .mass = 1.805635e21,
          .gm = 1.205e11,
          .rotation_period = 6859008.0,
          .albedo = 0.6,
      })
      .child_of(saturn);

  world.entity("Phoebe")
      .set<CelestialBody>({
          .semi_major_axis = 12952000000.0,
          .eccentricity = 0.1634,
          .inclination = 0.0,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = true,
          .radius = 106500.0,
          .gravity = 0.056,
          .density = 1634.0,
          .mass = 8.3e18,
          .gm = 5.533e8,
          .rotation_period = 52876800.0,
          .albedo = 0.081,
      })
      .child_of(saturn);

  world.entity("Janus")
      .set<CelestialBody>({
          .semi_major_axis = 151460000.0,
          .eccentricity = 0.0068,
          .inclination = 0.0,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 89340.0,
          .gravity = 0.023,
          .density = 630.0,
          .mass = 1.897e18,
          .gm = 1.267e8,
          .rotation_period = 59712.0,
          .albedo = 0.72,
      })
      .child_of(saturn);

  auto uranus = world.entity("Uranus")
                    .set<CelestialBody>({
                        .semi_major_axis = 2870658170655.7324,
                        .eccentricity = 0.04725744,
                        .inclination = 0.013485074058964219,
                        .longitude_of_ascending_node = 1.2918390439753027,
                        .argument_of_periapsis = 1.6918759478238068,
                        .mean_anomaly_at_epoch = 2.48332127460649,
                        .retrograde = false,
                        .radius = 25362000.0,
                        .gravity = 8.69,
                        .density = 1270.0,
                        .mass = 8.6810e25,
                        .gm = 5.793939e15,
                        .rotation_period = 62064.0,
                        .albedo = 0.3,
                    })
                    .child_of(sun);
  world.entity("Miranda")
      .set<CelestialBody>({
          .semi_major_axis = 129900000.0,
          .eccentricity = 0.0013,
          .inclination = 0.0,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 235800.0,
          .gravity = 0.079,
          .density = 1200.0,
          .mass = 6.59e19,
          .gm = 4.359e9,
          .rotation_period = 122400.0,
          .albedo = 0.32,
      })
      .child_of(uranus);

  world.entity("Ariel")
      .set<CelestialBody>({
          .semi_major_axis = 190900000.0,
          .eccentricity = 0.0012,
          .inclination = 0.0,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 578900.0,
          .gravity = 0.27,
          .density = 1660.0,
          .mass = 1.353e21,
          .gm = 9.14e10,
          .rotation_period = 95040.0,
          .albedo = 0.39,
      })
      .child_of(uranus);

  world.entity("Umbriel")
      .set<CelestialBody>({
          .semi_major_axis = 266000000.0,
          .eccentricity = 0.0039,
          .inclination = 0.0,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 584700.0,
          .gravity = 0.23,
          .density = 1390.0,
          .mass = 1.172e21,
          .gm = 7.915e10,
          .rotation_period = 131520.0,
          .albedo = 0.21,
      })
      .child_of(uranus);

  world.entity("Titania")
      .set<CelestialBody>({
          .semi_major_axis = 436300000.0,
          .eccentricity = 0.0011,
          .inclination = 0.0,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 788900.0,
          .gravity = 0.37,
          .density = 1710.0,
          .mass = 3.42e21,
          .gm = 2.278e11,
          .rotation_period = 209520.0,
          .albedo = 0.27,
      })
      .child_of(uranus);

  world.entity("Oberon")
      .set<CelestialBody>({
          .semi_major_axis = 583500000.0,
          .eccentricity = 0.0014,
          .inclination = 0.0,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 761400.0,
          .gravity = 0.35,
          .density = 1630.0,
          .mass = 3.014e21,
          .gm = 2.008e11,
          .rotation_period = 323040.0,
          .albedo = 0.23,
      })
      .child_of(uranus);

  auto neptune = world.entity("Neptune")
                     .set<CelestialBody>({
                         .semi_major_axis = 4498396417009.467,
                         .eccentricity = 0.00859048,
                         .inclination = 0.030893086454925476,
                         .longitude_of_ascending_node = 2.300068641354461,
                         .argument_of_periapsis = -1.5152854923664414,
                         .mean_anomaly_at_epoch = -1.746809150875549,
                         .retrograde = false,
                         .radius = 24622000.0,
                         .gravity = 11.15,
                         .density = 1638.0,
                         .mass = 1.02413e26,
                         .gm = 6.836529e15,
                         .rotation_period = 57996.0,
                         .albedo = 0.29,
                     })
                     .child_of(sun);
  world.entity("Triton")
      .set<CelestialBody>({
          .semi_major_axis = 354800000.0,
          .eccentricity = 0.000016,
          .inclination = 2.2656468085988792,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = true,
          .radius = 1353400.0,
          .gravity = 0.779,
          .density = 2061.0,
          .mass = 2.14e22,
          .gm = 1.4283002e12,
          .rotation_period = 507773.0,
          .albedo = 0.76,
      })
      .child_of(neptune);

  world.entity("Proteus")
      .set<CelestialBody>({
          .semi_major_axis = 117647000.0,
          .eccentricity = 0.0004,
          .inclination = 0.0,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 210000.0,
          .gravity = 0.075,
          .density = 1300.0,
          .mass = 5.0e19,
          .gm = 3.33715e9,
          .rotation_period = 96968.016,
          .albedo = 0.096,
      })
      .child_of(neptune);

  world.entity("Nereid")
      .set<CelestialBody>({
          .semi_major_axis = 5513400000.0,
          .eccentricity = 0.749,
          .inclination = 0.10122909661567112,
          .longitude_of_ascending_node = 5.689773361501514,
          .argument_of_periapsis = 5.066690818539539,
          .mean_anomaly_at_epoch = 5.550147021341968,
          .retrograde = false,
          .radius = 170000.0,
          .gravity = 0.071,
          .density = 1500.0,
          .mass = 3.57e19,
          .gm = 2.3827251e9,
          .rotation_period = 41738.4,
          .albedo = 0.24,
      })
      .child_of(neptune);

  world.entity("Eris")
      .set<CelestialBody>({
          .semi_major_axis = 10104000000000.0,
          .eccentricity = 0.44177,
          .inclination = 0.3820626331460562,
          .longitude_of_ascending_node = 2.003554379732539,
          .argument_of_periapsis = 2.069505947436491,
          .mean_anomaly_at_epoch = 1.2227287845950825,
          .retrograde = false,
          .radius = 1163000.0,
          .gravity = 0.82,
          .density = 2510.0,
          .mass = 1.6466e22,
          .gm = 1.108e12,
          .rotation_period = 2042784.0,
          .albedo = 0.67,
      })
      .child_of(sun);

  world.entity("Haumea")
      .set<CelestialBody>({
          .semi_major_axis = 6432000000000.0,
          .eccentricity = 0.19126,
          .inclination = 0.4373792740947354,
          .longitude_of_ascending_node = 1.078207684520758,
          .argument_of_periapsis = 0.706341025767512,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 816000.0,
          .gravity = 0.401,
          .density = 1951.0,
          .mass = 4.006e21,
          .gm = 2.668e11,
          .rotation_period = 14256.0,
          .albedo = 0.51,
      })
      .child_of(sun);

  world.entity("Makemake")
      .set<CelestialBody>({
          .semi_major_axis = 6857000000000.0,
          .eccentricity = 0.159,
          .inclination = 0.4363323129985824,
          .longitude_of_ascending_node = 5.073408080,
          .argument_of_periapsis = 3.282676,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 715000.0,
          .gravity = 0.5,
          .density = 1950.0,
          .mass = 3.1e21,
          .gm = 2.07e11,
          .rotation_period = 30600.0,
          .albedo = 0.77,
      })
      .child_of(sun);

  world.entity("Gonggong")
      .set<CelestialBody>({
          .semi_major_axis = 10142000000000.0,
          .eccentricity = 0.5005,
          .inclination = 0.24434609527920614,
          .longitude_of_ascending_node = 6.09119908946021,
          .argument_of_periapsis = 1.064650843716541,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 615000.0,
          .gravity = 0.25,
          .density = 1520.0,
          .mass = 1.75e21,
          .gm = 1.168e11,
          .rotation_period = 385200.0,
          .albedo = 0.27,
      })
      .child_of(sun);

  world.entity("Quaoar")
      .set<CelestialBody>({
          .semi_major_axis = 6560000000000.0,
          .eccentricity = 0.038,
          .inclination = 0.1972222053173537,
          .longitude_of_ascending_node = 4.886921905584122,
          .argument_of_periapsis = 4.1887902047863905,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 555000.0,
          .gravity = 0.3,
          .density = 1995.0,
          .mass = 1.4e21,
          .gm = 9.34e10,
          .rotation_period = 316800.0,
          .albedo = 0.19,
      })
      .child_of(sun);

  world.entity("Sedna")
      .set<CelestialBody>({
          .semi_major_axis = 75100000000000.0,
          .eccentricity = 0.849,
          .inclination = 0.41887902047863906,
          .longitude_of_ascending_node = 2.5830872929516078,
          .argument_of_periapsis = 2.4958208303518914,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 500000.0,
          .gravity = 0.4,
          .density = 2170.0,
          .mass = 1.0e21,
          .gm = 6.6743e10,
          .rotation_period = 36792000.0,
          .albedo = 0.32,
      })
      .child_of(sun);

  auto pluto = world.entity("Pluto")
                   .set<CelestialBody>({
                       .semi_major_axis = 5906440596528.805,
                       .eccentricity = 0.2488273,
                       .inclination = 0.29914964427853585,
                       .longitude_of_ascending_node = 1.92516687576987,
                       .argument_of_periapsis = 1.9855734648661874,
                       .mean_anomaly_at_epoch = 0.25935805684617685,
                       .retrograde = false,
                       .radius = 1188300.0,
                       .gravity = 0.62,
                       .density = 1854.0,
                       .mass = 1.303e22,
                       .gm = 8.71e11,
                       .rotation_period = 551856.70656,
                       .albedo = 0.72,
                   })
                   .child_of(sun);
  world.entity("Charon")
      .set<CelestialBody>({
          .semi_major_axis = 19596000.0,
          .eccentricity = 0.00005,
          .inclination = 0.0,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 606000.0,
          .gravity = 0.288,
          .density = 1701.0,
          .mass = 1.586e21,
          .gm = 8.964e9,
          .rotation_period = 6.38723 * 86400.0,
          .albedo = 0.38,
      })
      .child_of(pluto);

  world.entity("Styx")
      .set<CelestialBody>({
          .semi_major_axis = 42600000.0,
          .eccentricity = 0.0,
          .inclination = 0.0,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 16000.0,
          .gravity = 0.65,
          .density = 1.0e3,
          .mass = 5e14,
          .gm = 3.0e10,
          .rotation_period = 3.24 * 86400.0,
          .albedo = 0.65,
      })
      .child_of(pluto);

  world.entity("Nix")
      .set<CelestialBody>({
          .semi_major_axis = 48694000.0,
          .eccentricity = 0.0,
          .inclination = 0.0,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 24900.0,
          .gravity = 0.68,
          .density = 1.0e3,
          .mass = 2.7e16,
          .gm = 1.8e13,
          .rotation_period = 24.8546 * 86400.0,
          .albedo = 0.68,
      })
      .child_of(pluto);

  world.entity("Kerberos")
      .set<CelestialBody>({
          .semi_major_axis = 57783000.0,
          .eccentricity = 0.00328,
          .inclination = 0.0,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 19000.0,
          .gravity = 0.56,
          .density = 1.0e3,
          .mass = 8e14,
          .gm = 4.6e12,
          .rotation_period = 32.167 * 86400.0,
          .albedo = 0.56,
      })
      .child_of(pluto);

  world.entity("Hydra")
      .set<CelestialBody>({
          .semi_major_axis = 64721000.0,
          .eccentricity = 0.00554,
          .inclination = 0.0,
          .longitude_of_ascending_node = 0.0,
          .argument_of_periapsis = 0.0,
          .mean_anomaly_at_epoch = 0.0,
          .retrograde = false,
          .radius = 25600.0,
          .gravity = 0.83,
          .density = 1.2e3,
          .mass = 3.0e16,
          .gm = 2.0e13,
          .rotation_period = 38.2 * 86400.0,
          .albedo = 0.83,
      })
      .child_of(pluto);
}
