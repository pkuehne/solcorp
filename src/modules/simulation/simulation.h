#pragma once

#include <flecs.h>

struct Simulation {
  flecs::entity speed;
};

struct Game {
  u_int day = 0;
};

/// \brief Represents a celestial body with orbital and physical parameters.
/// All parameters are in SI units unless otherwise specified. Angles in radians
struct CelestialBody {
  // --- Orbital elements ---
  double semi_major_axis;             // metres
  double eccentricity;                // 0.0–1.0
  double inclination;                 // Radians
  double longitude_of_ascending_node; // Ω
  double argument_of_periapsis;       // ω
  double mean_anomaly_at_epoch;       // M₀
  bool retrograde = false;

  // --- Physical parameters ---
  double radius;          // metres
  double gravity;         // surface gravity (m/s²)
  double density;         // kg/m³
  double mass;            // kg
  double gm;              // m³/s² (gravitational parameter = G*M)
  double rotation_period; // seconds per rotation
  double albedo;          // 0.0–1.0
};

struct Developer {};

void systemRegisterWindows(flecs::iter &);
void systemGenerateSolSystem(flecs::iter &);

struct SimulationModule {
  SimulationModule(flecs::world &world);
};
