#include "modules/base/base.h"
#include "modules/engine/engine.h"
#include "modules/lua/helpers.h"
#include "modules/rocket/rocket_module.h"
#include "modules/site/site.h"
#include "modules/stats/stats.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

static void run_site_prefab_setup(flecs::world &world) {
  world.import <SiteModule>();
  auto sys = world.system("Setup Site Prefabs")
                 .kind(flecs::OnStart)
                 .immediate()
                 .run(systemCreateSitePrefabs);
  sys.run();
}

static void run_rocket_prefab_setup(flecs::world &world) {
  world.import <RocketModule>();
  auto sys = world.system("Setup Rocket Prefabs")
                 .kind(flecs::OnStart)
                 .immediate()
                 .run(systemCreateRocketPrefabs);
  sys.run();
}

// ── create_site ───────────────────────────────────────────────────────────

SCENARIO("create_site", "[helpers][lua]") {
  flecs::world world;
  world.import <SiteModule>();
  world.entity("Earth").child_of(world.entity("Sun"));

  GIVEN("a name and dimensions") {
    WHEN("created without make_current") {
      auto site = create_site(world, "Alpha", 10, 20);
      THEN("entity is valid with correct dimensions") {
        REQUIRE(site.is_valid());
        auto &s = site.get<Site>();
        CHECK(s.width == 10);
        CHECK(s.height == 20);
      }
      THEN("entity has ConstructionSiteNeedsUpdating") {
        CHECK(site.has<ConstructionSiteNeedsUpdating>());
      }
      THEN("entity does not have CurrentSite") {
        CHECK(!site.has<CurrentSite>());
      }
    }

    WHEN("created with make_current = true") {
      auto site = create_site(world, "Beta", 5, 5, true);
      THEN("entity has CurrentSite") { CHECK(site.has<CurrentSite>()); }
    }
  }
}

// ── create_building_prefab ─────────────────────────────────────────────────

SCENARIO("create_building_prefab", "[helpers][lua]") {
  flecs::world world;
  run_site_prefab_setup(world);

  GIVEN("a prefab name") {
    WHEN("create_building_prefab is called") {
      auto prefab = create_building_prefab(world, "ControlCenter");
      THEN("a valid prefab entity is returned") { REQUIRE(prefab.is_valid()); }
      THEN("it is a child of Prefabs::Buildings") {
        CHECK(prefab.parent() == world.lookup("Prefabs::Buildings"));
      }
    }
  }
}

// ── create_rocket_prefab ────────────────────────────────────────────────────

SCENARIO("create_rocket_prefab", "[helpers][lua]") {
  flecs::world world;
  run_rocket_prefab_setup(world);

  GIVEN("a prefab name") {
    WHEN("create_rocket_prefab is called") {
      auto prefab = create_rocket_prefab(world, "Falcon9");
      THEN("a valid prefab entity is returned") { REQUIRE(prefab.is_valid()); }
      THEN("it is a child of Prefabs::Rockets") {
        CHECK(prefab.parent() == world.lookup("Prefabs::Rockets"));
      }
    }
  }
}

// ── add_facility_to_building ────────────────────────────────────────────────

SCENARIO("add_facility_to_building", "[helpers][lua]") {
  flecs::world world;
  run_site_prefab_setup(world);
  auto building = world.entity("HQ");

  GIVEN("a valid building entity") {
    WHEN("add_facility_to_building is called") {
      auto facility =
          add_facility_to_building(world, building, "MissionControl");
      THEN("a valid entity is returned") { REQUIRE(facility.is_valid()); }
      THEN("it is a child of the building") {
        CHECK(facility.parent() == building);
      }
    }
  }
}

// ── create_rocket ───────────────────────────────────────────────────────────

SCENARIO("create_rocket", "[helpers][lua]") {
  flecs::world world;
  run_rocket_prefab_setup(world);
  create_rocket_prefab(world, "TestBooster");

  GIVEN("a valid prefab name") {
    WHEN("create_rocket is called without a parent") {
      auto rocket = create_rocket(world, RocketName{"Booster1"},
                                  RocketPrefabType{"TestBooster"});
      THEN("a valid entity with the given name is returned") {
        REQUIRE(rocket.is_valid());
        CHECK(std::string(rocket.name().c_str()) == "Booster1");
      }
    }

    WHEN("create_rocket is called with a parent entity") {
      auto parent = world.entity("LaunchPad");
      auto rocket = create_rocket(world, RocketName{"Booster2"},
                                  RocketPrefabType{"TestBooster"}, parent);
      THEN("the rocket is a child of the parent") {
        REQUIRE(rocket.is_valid());
        CHECK(rocket.parent() == parent);
      }
    }
  }

  GIVEN("a prefab name that does not exist") {
    WHEN("create_rocket is called") {
      auto rocket = create_rocket(world, RocketName{"Ghost"},
                                  RocketPrefabType{"NoSuchPrefab"});
      THEN("an invalid entity is returned") { CHECK(!rocket.is_valid()); }
    }
  }
}

// ── create_building ──────────────────────────────────────────────────────────

SCENARIO("create_building", "[helpers][lua]") {
  flecs::world world;
  run_site_prefab_setup(world);
  create_building_prefab(world, "Warehouse");
  auto site = world.entity("SiteA");

  GIVEN("a valid prefab name and site") {
    WHEN("create_building is called") {
      auto building = create_building(world, "WH1", "Warehouse", 3, 4, site);
      THEN("a valid entity is returned") { REQUIRE(building.is_valid()); }
      THEN("it is a child of the site") { CHECK(building.parent() == site); }
    }
  }

  GIVEN("a prefab name that does not exist") {
    WHEN("create_building is called") {
      auto building =
          create_building(world, "Ghost", "NoSuchPrefab", 0, 0, site);
      THEN("an invalid entity is returned") { CHECK(!building.is_valid()); }
    }
  }
}

// ── add_target_orbit_to_rocket ───────────────────────────────────────────

SCENARIO("add_target_orbit_to_rocket", "[helpers][lua]") {
  flecs::world world;
  world.import <RocketModule>();
  auto rocket = world.entity("RocketLEO").add<Rocket>();
  world.entity("LEO");

  GIVEN("a rocket and an existing orbit name") {
    WHEN("add_target_orbit_to_rocket is called") {
      add_target_orbit_to_rocket(world, rocket, "LEO", 7500);
      THEN("rocket has CanLiftTo targeting the orbit with correct max_mass") {
        auto orbit = world.lookup("LEO");
        REQUIRE(rocket.has<CanLiftTo>(orbit));
        CHECK(rocket.get<CanLiftTo>(orbit).max_mass == 7500);
      }
    }
  }
}

// ── create_texture ──────────────────────────────────────────────────────

SCENARIO("create_texture", "[helpers][lua]") {
  flecs::world world;
  world.import <BaseModule>();
  registerEngineComponents(world);
  world.entity("Textures");

  GIVEN("a filename containing '..'") {
    WHEN("create_texture is called") {
      auto e = create_texture(world, TextureName{"Bad"},
                              TextureFilename{"../secret.png"},
                              TextureModName{"core"});
      THEN("an invalid entity is returned") { CHECK(!e.is_valid()); }
    }
  }

  GIVEN("a valid filename (no renderer, texture ptr will be null)") {
    WHEN("create_texture is called") {
      auto e =
          create_texture(world, TextureName{"Sheet"},
                         TextureFilename{"sheet.png"}, TextureModName{"core"});
      THEN("an entity is created as a child of Textures") {
        REQUIRE(e.is_valid());
        CHECK(e.parent() == world.lookup("Textures"));
      }
    }
  }
}

// ── create_effect ───────────────────────────────────────────────────────

SCENARIO("create_effect", "[helpers][lua]") {
  flecs::world world;
  world.import <StatsModule>();

  GIVEN("a name with no source entity") {
    WHEN("create_effect is called") {
      auto effect = create_effect(world, "BurnEffect", flecs::entity());
      THEN("a valid entity with Effect component is created") {
        REQUIRE(effect.is_valid());
        CHECK(effect.has<Effect>());
      }
      THEN("effect is a child of the Effects node") {
        CHECK(effect.parent() == world.lookup("Effects"));
      }
    }
  }

  GIVEN("a name and a valid source entity") {
    WHEN("create_effect is called") {
      auto source = world.entity("Booster");
      auto effect = create_effect(world, "ThrustEffect", source);
      THEN("source has HasEffect pointing to the new effect") {
        REQUIRE(effect.is_valid());
        CHECK(source.has<HasEffect>(effect));
      }
    }
  }
}

// ── add_modifier ──────────────────────────────────────────────────────

SCENARIO("add_modifier", "[helpers][lua]") {
  flecs::world world;
  world.import <StatsModule>();

  GIVEN("a valid effect entity") {
    WHEN("add_modifier is called") {
      auto effect = world.entity("SpeedEffect").add<Effect>();
      Modifier mod{
          .target_stat = "thrust", .additive = 100.0, .multiplicative = 1.5};
      auto modifier = add_modifier(world, effect, mod);
      THEN("a valid modifier entity is returned") {
        REQUIRE(modifier.is_valid());
        CHECK(modifier.has<Modifier>());
      }
      THEN("modifier is a child of the effect") {
        CHECK(modifier.parent() == effect);
      }
      THEN("modifier data is stored correctly") {
        auto &m = modifier.get<Modifier>();
        CHECK(m.target_stat == "thrust");
        CHECK(m.additive == 100.0);
        CHECK(m.multiplicative == 1.5);
      }
    }
  }

  GIVEN("an invalid effect entity") {
    WHEN("add_modifier is called") {
      auto modifier = add_modifier(world, flecs::entity(), Modifier{});
      THEN("an invalid entity is returned") { CHECK(!modifier.is_valid()); }
    }
  }
}

// ── clip_sprite_from_texture ──────────────────────────────────────────────

SCENARIO("clip_sprite_from_texture", "[helpers][lua]") {
  flecs::world world;
  world.import <BaseModule>();
  registerEngineComponents(world);

  GIVEN("a texture name that does not exist") {
    WHEN("clip_sprite_from_texture is called") {
      auto sprite = clip_sprite_from_texture(
          world, "missing",
          SpriteClipRect{.x = 0, .y = 0, .width = 32, .height = 32});
      THEN("an empty Sprite is returned") {
        CHECK(sprite.texture == flecs::entity());
      }
    }
  }

  GIVEN("a texture entity exists in the Textures hierarchy") {
    auto textures = world.entity("Textures");
    auto tex = world.entity("sheet").child_of(textures);

    WHEN("clip_sprite_from_texture is called with a clipping region") {
      auto sprite = clip_sprite_from_texture(
          world, "sheet",
          SpriteClipRect{.x = 10, .y = 20, .width = 64, .height = 48});
      THEN("the sprite references the texture entity") {
        CHECK(sprite.texture == tex);
      }
      THEN("the clip coordinates are set correctly") {
        CHECK(sprite.x == 10);
        CHECK(sprite.y == 20);
        CHECK(sprite.width == 64);
        CHECK(sprite.height == 48);
      }
    }
  }
}

// ── create_contract ─────────────────────────────────────────────────────

SCENARIO("create_contract", "[helpers][lua]") {
  flecs::world world;
  world.import <RocketModule>();
  world.entity("Contracts");

  GIVEN("a unique contract name") {
    WHEN("create_contract is called") {
      auto contract = create_contract(world, "Mission1", "AcmeCorp",
                                      "Deliver satellite", 2000.0f, 8000.0f);
      THEN("a valid entity is returned") { REQUIRE(contract.is_valid()); }
      THEN("the Contract component has the correct fields") {
        auto &c = contract.get<Contract>();
        CHECK(c.client == "AcmeCorp");
        CHECK(c.description == "Deliver satellite");
        CHECK(c.upfront_payment == 2000.0f);
        CHECK(c.completion_payment == 8000.0f);
      }
      THEN("the entity is a child of the Contracts node") {
        CHECK(contract.parent() == world.lookup("Contracts"));
      }
    }
  }

  GIVEN("a name that already exists under Contracts") {
    auto existing = world.entity("Dup")
                        .child_of(world.lookup("Contracts"))
                        .set<Contract>({.client = "OldClient",
                                        .description = "OldDesc",
                                        .upfront_payment = 0,
                                        .completion_payment = 0,
                                        .status = ContractStatus::Open,
                                        .failed = false});
    WHEN("create_contract is called with the same name") {
      auto contract =
          create_contract(world, "Dup", "NewClient", "NewDesc", 0, 0);
      THEN("the existing entity is returned") { CHECK(contract == existing); }
    }
  }
}

// ── create_contract_payload ───────────────────────────────────────────────

SCENARIO("create_contract_payload", "[helpers][lua]") {
  flecs::world world;
  world.import <RocketModule>();
  auto contract = world.entity("ContractX")
                      .set<Contract>({.client = "C",
                                      .description = "D",
                                      .upfront_payment = 0,
                                      .completion_payment = 0,
                                      .status = ContractStatus::Open,
                                      .failed = false});

  GIVEN("a contract, payload name, and no orbit") {
    WHEN("create_contract_payload is called with an empty orbit name") {
      auto payload = create_contract_payload(world, contract, "SatA", 500, "");
      THEN("a valid payload entity is returned") {
        REQUIRE(payload.is_valid());
        CHECK(payload.get<Payload>().mass == 500);
      }
      THEN("payload is a child of the contract") {
        CHECK(payload.parent() == contract);
      }
      THEN("contract has ContractPayload pointing to payload") {
        CHECK(contract.has<ContractPayload>(payload));
      }
      THEN("no ContractTargetOrbit is set") {
        CHECK(!contract.has<ContractTargetOrbit>());
      }
    }
  }

  GIVEN("a contract and a valid orbit name") {
    world.entity("GTO");
    WHEN("create_contract_payload is called with the orbit name") {
      create_contract_payload(world, contract, "SatB", 1200, "GTO");
      THEN("ContractTargetOrbit is set to the orbit entity") {
        auto orbit = world.lookup("GTO");
        CHECK(contract.has<ContractTargetOrbit>(orbit));
      }
    }
  }

  GIVEN("a contract and an orbit name that does not exist") {
    WHEN("create_contract_payload is called") {
      auto payload =
          create_contract_payload(world, contract, "SatC", 300, "Nonexistent");
      THEN("payload is still created") { CHECK(payload.is_valid()); }
      THEN("no ContractTargetOrbit is set") {
        CHECK(!contract.has<ContractTargetOrbit>());
      }
    }
  }
}

// ── get_all_contracts ────────────────────────────────────────────────────

SCENARIO("get_all_contracts", "[helpers][lua]") {
  flecs::world world;
  world.import <RocketModule>();

  GIVEN("no contracts in the world") {
    WHEN("get_all_contracts is called") {
      auto result = get_all_contracts(world);
      THEN("an empty vector is returned") { CHECK(result.empty()); }
    }
  }

  GIVEN("three contracts in the world") {
    world.entity("C1").set<Contract>({.client = "Client",
                                      .description = "Desc",
                                      .upfront_payment = 0,
                                      .completion_payment = 0,
                                      .status = ContractStatus::Open,
                                      .failed = false});
    world.entity("C2").set<Contract>({.client = "Client",
                                      .description = "Desc",
                                      .upfront_payment = 0,
                                      .completion_payment = 0,
                                      .status = ContractStatus::Open,
                                      .failed = false});
    world.entity("C3").set<Contract>({.client = "Client",
                                      .description = "Desc",
                                      .upfront_payment = 0,
                                      .completion_payment = 0,
                                      .status = ContractStatus::Open,
                                      .failed = false});
    WHEN("get_all_contracts is called") {
      auto result = get_all_contracts(world);
      THEN("all three are returned") { CHECK(result.size() == 3); }
    }
  }
}

// ── get_all_active_contracts ────────────────────────────────────────────────

SCENARIO("get_all_active_contracts", "[helpers][lua]") {
  flecs::world world;
  world.import <RocketModule>();

  GIVEN("contracts with Open, Accepted, and Closed statuses") {
    world.entity("Open1").set<Contract>({.client = "C",
                                         .description = "D",
                                         .upfront_payment = 0,
                                         .completion_payment = 0,
                                         .status = ContractStatus::Open,
                                         .failed = false});
    world.entity("Open2").set<Contract>({.client = "C",
                                         .description = "D",
                                         .upfront_payment = 0,
                                         .completion_payment = 0,
                                         .status = ContractStatus::Open,
                                         .failed = false});
    world.entity("Accepted1")
        .set<Contract>({.client = "C",
                        .description = "D",
                        .upfront_payment = 0,
                        .completion_payment = 0,
                        .status = ContractStatus::Accepted,
                        .failed = false});
    world.entity("Closed1").set<Contract>({.client = "C",
                                           .description = "D",
                                           .upfront_payment = 0,
                                           .completion_payment = 0,
                                           .status = ContractStatus::Closed,
                                           .failed = false});
    world.entity("Closed2").set<Contract>({.client = "C",
                                           .description = "D",
                                           .upfront_payment = 0,
                                           .completion_payment = 0,
                                           .status = ContractStatus::Closed,
                                           .failed = false});

    WHEN("get_all_active_contracts is called") {
      auto result = get_all_active_contracts(world);
      THEN("only Open and Accepted contracts are returned") {
        CHECK(result.size() == 3);
      }
    }
  }

  GIVEN("only closed contracts") {
    world.entity("ClosedOnly")
        .set<Contract>({.client = "C",
                        .description = "D",
                        .upfront_payment = 0,
                        .completion_payment = 0,
                        .status = ContractStatus::Closed,
                        .failed = false});
    WHEN("get_all_active_contracts is called") {
      auto result = get_all_active_contracts(world);
      THEN("an empty vector is returned") { CHECK(result.empty()); }
    }
  }
}
