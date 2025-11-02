#include "simulation.h"
#include "SDL_keycode.h"
#include "modules/base/base.h"
#include "modules/engine/input.h"
#include "modules/lua/lua.h"
#include "modules/simulation/developer_window.h"
#include "spdlog/spdlog.h"

void systemUpdateSimDate(Game &game);
void systemQuitOnEscape(flecs::iter &, size_t, const KeyDown);
void systemShowDeveloperWindow(flecs::iter &, size_t, const KeyDown);
void systemModCallbackForUpdate(flecs::entity, Mod &);
void systemModCallbackForFrame(flecs::entity, Mod &);

SimulationModule::SimulationModule(flecs::world &world) {
  // Register components
  world.component<Simulation>()
      .member("speed", &Simulation::speed)
      .add(flecs::Singleton);
  world.component<Game>().member("day", &Game::day).add(flecs::Singleton);
  world.component<Developer>()
      .member("show_metrics_window", &Developer::show_metrics_window)
      .add(flecs::Singleton);
  world.component<DeveloperWindow>().member("open", &DeveloperWindow::open);

  // Create Singletons
  auto sim = Simulation{world.timer("SimTimer").interval(0.5f).disable()};
  world.set<Simulation>(sim);
  world.set<Game>({});
  world.set<Developer>({});

  register_lua_user_type<Game>(
      world, "Game",
      [](sol::usertype<Game> &userType) { userType["day"] = &Game::day; });

  // Register systems
  world.system<Game>("Update Simulation Date")
      .tick_source(sim.speed)
      .kind(UpdatePhase)
      .each(systemUpdateSimDate);

  world.system<const KeyDown>("Quit on Esc")
      .kind(ValidatePhase)
      .each(systemQuitOnEscape);
  world.system<const KeyDown>("Show Developer Window")
      .kind(ValidatePhase)
      .each(systemShowDeveloperWindow);
  world.system<DeveloperWindow>("Draw Developer Window")
      .kind(GuiPhase)
      .each(systemDrawDeveloperWindow);
  world.system<Mod>("Mod on_update Event")
      .kind(UpdatePhase)
      .tick_source(sim.speed)
      .each(systemModCallbackForUpdate);
  world.system<Mod>("Mod on_frame Event")
      .kind(UpdatePhase)
      .each(systemModCallbackForFrame);
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

void systemShowDeveloperWindow(flecs::iter &it, size_t, const KeyDown event) {
  auto world = it.world();
  if (event.key == SDLK_d) {
    showDeveloperWindow(world);
  }
}

void systemModCallbackForUpdate(flecs::entity e, Mod &mod) {
  auto world = e.world();

  run_mod_handler(mod, world, "on_update");
}

void systemModCallbackForFrame(flecs::entity e, Mod &mod) {
  auto world = e.world();

  run_mod_handler(mod, world, "on_frame");
}

void systemGenerateSolSystem(flecs::iter &it) {
  spdlog::debug("Generating Sol System...");
  auto world = it.world();

  auto sun = world.entity("Sun").set<CelestialBody>(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, false, 695700000.0, 274.0, 1408.0,
       1.9885e30, 1.32712440018e20, 2192832.0, 0.0});

  world.entity("Mercury")
      .set<CelestialBody>({57909226541.52439, 0.20563593, 0.12225994793212572,
                           0.8435309949656006, 0.5083625818084809,
                           3.050705116248391, false, 2439700.0, 3.7, 5427.0,
                           3.3011e23, 2.2032e13, 5067033.84, 0.088})
      .child_of(sun);

  world.entity("Venus")
      .set<CelestialBody>({108209474537.37917, 0.00677672, 0.05924827411109566,
                           1.3383157224083446, 0.9585806304888396,
                           0.8792381031921823, false, 6051800.0, 8.87, 5243.0,
                           4.8675e24, 3.24858599e14, 20997360.0, 0.75})
      .child_of(sun);

  auto earth =
      world.entity("Earth")
          .set<CelestialBody>(
              {149598261150.4425, 0.01671123, -0.00000026720990848033185, 0.0,
               1.7966014740491711, -0.04316391697638581, false, 6371000.0,
               9.80665, 5514.0, 5.97237e24, 3.986004418e14, 86164.0905, 0.306})
          .child_of(sun);
  world.entity("Moon")
      .set<CelestialBody>({384400000.0, 0.0549, 0.08979719001534528,
                           2.183259703385116, 3.371273012690141,
                           2.013245780213528, false, 1737400.0, 1.62, 3344.0,
                           7.342e22, 4.902801e12, 2360591.0, 0.11})
      .child_of(earth);

  auto mars = world.entity("Mars")
                  .set<CelestialBody>(
                      {227939366344.5385, 0.0933941, 0.03228859115900383,
                       0.8649512068711695, -1.2877332456216607,
                       0.3386557814120793, false, 3389500.0, 3.711, 3933.0,
                       6.4171e23, 4.282837e13, 88642.6632, 0.25})
                  .child_of(sun);
  world.entity("Phobos")
      .set<CelestialBody>({9376000.0, 0.0151, 0.0, 0.0, 0.0, 0.0, false,
                           11266.7, 0.0057, 1860.0, 1.0659e16, 7.087e5, 27552.0,
                           0.07})
      .child_of(mars);

  world.entity("Deimos")
      .set<CelestialBody>({23463000.0, 0.00033, 0.0, 0.0, 0.0, 0.0, false,
                           6200.0, 0.003, 1470.0, 1.4762e15, 9.615e4, 109080.0,
                           0.068})
      .child_of(mars);

  world.entity("Ceres")
      .set<CelestialBody>({414010000000.0, 0.075822, 0.1849586447958657,
                           1.333289568682072, 2.987938064617217,
                           5.98085718540448, false, 473000.0, 0.27, 2160.0,
                           9.393e20, 6.26325e10, 32668.8, 0.09})
      .child_of(sun);

  world.entity("Pallas")
      .set<CelestialBody>({414970000000.0, 0.23067, 0.37049525105022886,
                           1.7002899450667904, 3.354619380504511,
                           3.053859271012634, false, 256000.0, 0.23, 2900.0,
                           2.11e20, 1.4102e10, 107798.4, 0.15})
      .child_of(sun);

  world.entity("Vesta")
      .set<CelestialBody>({353260000000.0, 0.08874, 0.12232045072870662,
                           1.703305962, 2.036937137, 5.963, false, 262700.0,
                           0.25, 3456.0, 2.59076e20, 1.728e10, 18835.2, 0.42})
      .child_of(sun);

  auto jupiter = world.entity("Jupiter")
                     .set<CelestialBody>(
                         {778340821344.9, 0.04838624, 0.022767601968366663,
                          1.7534314474965515, -1.4990269948612492,
                          0.3429473263260368, false, 69911000.0, 24.79, 1326.0,
                          1.8982e27, 1.26686534e17, 35730.0, 0.343})
                     .child_of(sun);
  world.entity("Io")
      .set<CelestialBody>({421800000.0, 0.0041, 0.0, 0.0, 0.0, 0.0, false,
                           1821500.0, 1.796, 3528.0, 8.9319e22, 5.9599e12,
                           152853.504, 0.63})
      .child_of(jupiter);

  world.entity("Europa")
      .set<CelestialBody>({671100000.0, 0.009, 0.0, 0.0, 0.0, 0.0, false,
                           1560800.0, 1.315, 3013.0, 4.7998e22, 3.2027e12,
                           306822.336, 0.67})
      .child_of(jupiter);

  world.entity("Ganymede")
      .set<CelestialBody>({1070400000.0, 0.0013, 0.0, 0.0, 0.0, 0.0, false,
                           2634100.0, 1.428, 1942.0, 1.4819e23, 9.8878e12,
                           618153.6, 0.43})
      .child_of(jupiter);

  world.entity("Callisto")
      .set<CelestialBody>({1882700000.0, 0.0074, 0.0, 0.0, 0.0, 0.0, false,
                           2410300.0, 1.235, 1834.0, 1.0759e23, 7.1793e12,
                           1441939.2, 0.22})
      .child_of(jupiter);

  auto saturn = world.entity("Saturn")
                    .set<CelestialBody>(
                        {1426666422006.9587, 0.05386179, 0.04338874330931084,
                         1.9837835429754036, -0.36762823281234114,
                         -0.744289273004183, false, 58232000.0, 10.44, 687.0,
                         5.6834e26, 3.7931187e16, 38361.6, 0.342})
                    .child_of(sun);
  world.entity("Mimas")
      .set<CelestialBody>({185520000.0, 0.0196, 0.0, 0.0, 0.0, 0.0, false,
                           198200.0, 0.064, 1150.0, 3.7493e19, 2.504e9, 81492.0,
                           0.6})
      .child_of(saturn);

  world.entity("Enceladus")
      .set<CelestialBody>({238020000.0, 0.0047, 0.0, 0.0, 0.0, 0.0, false,
                           252100.0, 0.113, 1608.0, 1.08022e20, 7.210e9,
                           118320.0, 0.81})
      .child_of(saturn);

  world.entity("Tethys")
      .set<CelestialBody>({294670000.0, 0.0001, 0.0, 0.0, 0.0, 0.0, false,
                           531100.0, 0.145, 984.0, 6.17449e20, 4.008e10,
                           190080.0, 0.8})
      .child_of(saturn);

  world.entity("Dione")
      .set<CelestialBody>({377400000.0, 0.0022, 0.0, 0.0, 0.0, 0.0, false,
                           561400.0, 0.232, 1480.0, 1.095452e21, 7.311e10,
                           236160.0, 0.7})
      .child_of(saturn);

  world.entity("Rhea")
      .set<CelestialBody>({527040000.0, 0.0010, 0.0, 0.0, 0.0, 0.0, false,
                           763800.0, 0.264, 1230.0, 2.306518e21, 1.539e11,
                           390960.0, 0.6})
      .child_of(saturn);

  world.entity("Titan")
      .set<CelestialBody>({1221870000.0, 0.0288, 0.0, 0.0, 0.0, 0.0, false,
                           2575000.0, 1.352, 1880.0, 1.3452e23, 8.978e12,
                           1377648.0, 0.22})
      .child_of(saturn);

  world.entity("Hyperion")
      .set<CelestialBody>({1481100000.0, 0.1042, 0.0, 0.0, 0.0, 0.0, false,
                           133000.0, 0.056, 544.0, 5.6e18, 3.736e8, 1847520.0,
                           0.3})
      .child_of(saturn);

  world.entity("Iapetus")
      .set<CelestialBody>({3561300000.0, 0.0286, 0.0, 0.0, 0.0, 0.0, false,
                           734500.0, 0.223, 1088.0, 1.805635e21, 1.205e11,
                           6859008.0, 0.6})
      .child_of(saturn);

  world.entity("Phoebe")
      .set<CelestialBody>({12952000000.0, 0.1634, 0.0, 0.0, 0.0, 0.0, true,
                           106500.0, 0.056, 1634.0, 8.3e18, 5.533e8, 52876800.0,
                           0.081})
      .child_of(saturn);

  world.entity("Janus")
      .set<CelestialBody>({151460000.0, 0.0068, 0.0, 0.0, 0.0, 0.0, false,
                           89340.0, 0.023, 630.0, 1.897e18, 1.267e8, 59712.0,
                           0.72})
      .child_of(saturn);

  auto uranus =
      world.entity("Uranus")
          .set<CelestialBody>(
              {2870658170655.7324, 0.04725744, 0.013485074058964219,
               1.2918390439753027, 1.6918759478238068, 2.48332127460649, false,
               25362000.0, 8.69, 1270.0, 8.6810e25, 5.793939e15, 62064.0, 0.3})
          .child_of(sun);
  world.entity("Miranda")
      .set<CelestialBody>({129900000.0, 0.0013, 0.0, 0.0, 0.0, 0.0, false,
                           235800.0, 0.079, 1200.0, 6.59e19, 4.359e9, 122400.0,
                           0.32})
      .child_of(uranus);

  world.entity("Ariel")
      .set<CelestialBody>({190900000.0, 0.0012, 0.0, 0.0, 0.0, 0.0, false,
                           578900.0, 0.27, 1660.0, 1.353e21, 9.14e10, 95040.0,
                           0.39})
      .child_of(uranus);

  world.entity("Umbriel")
      .set<CelestialBody>({266000000.0, 0.0039, 0.0, 0.0, 0.0, 0.0, false,
                           584700.0, 0.23, 1390.0, 1.172e21, 7.915e10, 131520.0,
                           0.21})
      .child_of(uranus);

  world.entity("Titania")
      .set<CelestialBody>({436300000.0, 0.0011, 0.0, 0.0, 0.0, 0.0, false,
                           788900.0, 0.37, 1710.0, 3.42e21, 2.278e11, 209520.0,
                           0.27})
      .child_of(uranus);

  world.entity("Oberon")
      .set<CelestialBody>({583500000.0, 0.0014, 0.0, 0.0, 0.0, 0.0, false,
                           761400.0, 0.35, 1630.0, 3.014e21, 2.008e11, 323040.0,
                           0.23})
      .child_of(uranus);

  auto neptune = world.entity("Neptune")
                     .set<CelestialBody>(
                         {4498396417009.467, 0.00859048, 0.030893086454925476,
                          2.300068641354461, -1.5152854923664414,
                          -1.746809150875549, false, 24622000.0, 11.15, 1638.0,
                          1.02413e26, 6.836529e15, 57996.0, 0.29})
                     .child_of(sun);
  world.entity("Triton")
      .set<CelestialBody>({354800000.0, 0.000016, 2.2656468085988792, 0.0, 0.0,
                           0.0, true, 1353400.0, 0.779, 2061.0, 2.14e22,
                           1.4283002e12, 507773.0, 0.76})
      .child_of(neptune);

  world.entity("Proteus")
      .set<CelestialBody>({117647000.0, 0.0004, 0.0, 0.0, 0.0, 0.0, false,
                           210000.0, 0.075, 1300.0, 5.0e19, 3.33715e9,
                           96968.016, 0.096})
      .child_of(neptune);

  world.entity("Nereid")
      .set<CelestialBody>({5513400000.0, 0.749, 0.10122909661567112,
                           5.689773361501514, 5.066690818539539,
                           5.550147021341968, false, 170000.0, 0.071, 1500.0,
                           3.57e19, 2.3827251e9, 41738.4, 0.24})
      .child_of(neptune);

  world.entity("Eris")
      .set<CelestialBody>({10104000000000.0, 0.44177, 0.3820626331460562,
                           2.003554379732539, 2.069505947436491,
                           1.2227287845950825, false, 1163000.0, 0.82, 2510.0,
                           1.6466e22, 1.108e12, 2042784.0, 0.67})
      .child_of(sun);

  world.entity("Haumea")
      .set<CelestialBody>({6432000000000.0, 0.19126, 0.4373792740947354,
                           1.078207684520758, 0.706341025767512, 0.0, false,
                           816000.0, 0.401, 1951.0, 4.006e21, 2.668e11, 14256.0,
                           0.51})
      .child_of(sun);

  world.entity("Makemake")
      .set<CelestialBody>({6857000000000.0, 0.159, 0.4363323129985824,
                           5.073408080, 3.282676, 0.0, false, 715000.0, 0.5,
                           1950.0, 3.1e21, 2.07e11, 30600.0, 0.77})
      .child_of(sun);

  world.entity("Gonggong")
      .set<CelestialBody>({10142000000000.0, 0.5005, 0.24434609527920614,
                           6.09119908946021, 1.064650843716541, 0.0, false,
                           615000.0, 0.25, 1520.0, 1.75e21, 1.168e11, 385200.0,
                           0.27})
      .child_of(sun);

  world.entity("Quaoar")
      .set<CelestialBody>({6560000000000.0, 0.038, 0.1972222053173537,
                           4.886921905584122, 4.1887902047863905, 0.0, false,
                           555000.0, 0.3, 1995.0, 1.4e21, 9.34e10, 316800.0,
                           0.19})
      .child_of(sun);

  world.entity("Sedna")
      .set<CelestialBody>({75100000000000.0, 0.849, 0.41887902047863906,
                           2.5830872929516078, 2.4958208303518914, 0.0, false,
                           500000.0, 0.4, 2170.0, 1.0e21, 6.6743e10, 36792000.0,
                           0.32})
      .child_of(sun);

  auto pluto =
      world.entity("Pluto")
          .set<CelestialBody>(
              {5906440596528.805, 0.2488273, 0.29914964427853585,
               1.92516687576987, 1.9855734648661874, 0.25935805684617685, false,
               1188300.0, 0.62, 1854.0, 1.303e22, 8.71e11, 551856.70656, 0.72})
          .child_of(sun);
  world.entity("Charon")
      .set<CelestialBody>({19596000.0, 0.00005, 0.0, 0.0, 0.0, 0.0, false,
                           606000.0, 0.288, 1701.0, 1.586e21, 8.964e9,
                           6.38723 * 86400.0, 0.38})
      .child_of(pluto);

  world.entity("Styx")
      .set<CelestialBody>({42600000.0, 0.0, 0.0, 0.0, 0.0, 0.0, false, 16000.0,
                           0.65, 1.0e3, 5e14, 3.0e10, 3.24 * 86400.0, 0.65})
      .child_of(pluto);

  world.entity("Nix")
      .set<CelestialBody>({48694000.0, 0.0, 0.0, 0.0, 0.0, 0.0, false, 24900.0,
                           0.68, 1.0e3, 2.7e16, 1.8e13, 24.8546 * 86400.0,
                           0.68})
      .child_of(pluto);

  world.entity("Kerberos")
      .set<CelestialBody>({57783000.0, 0.00328, 0.0, 0.0, 0.0, 0.0, false,
                           19000.0, 0.56, 1.0e3, 8e14, 4.6e12, 32.167 * 86400.0,
                           0.56})
      .child_of(pluto);

  world.entity("Hydra")
      .set<CelestialBody>({64721000.0, 0.00554, 0.0, 0.0, 0.0, 0.0, false,
                           25600.0, 0.83, 1.2e3, 3.0e16, 2.0e13, 38.2 * 86400.0,
                           0.83})
      .child_of(pluto);
}
