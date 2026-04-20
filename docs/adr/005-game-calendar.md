# 005 — Game Calendar System

**Status:** Accepted

## Context

Time is currently tracked as a plain integer day counter. This is sufficient for advancing the simulation but breaks down when the game needs to:

- Display human-readable dates ("July 16, 1969") instead of "Day 1, Day 2"
- Distinguish weekdays from weekends so that construction and staffing tasks use business days, not calendar days
- Let plugins choose any historical or fictional start date (space race 1969, near-future 2045, etc.)
- Serialize and reload game state without losing date context

A naked `int` carries none of this information, and adding ad-hoc date math scattered across systems would make the logic untestable and error-prone.

## Decision

Introduce a `GameCalendar` Flecs component stored on the game entity. The component holds the current day index and the start date as plain integers, with member functions for all calendar operations.

```cpp
struct GameCalendar {
    int current_day;   // 0-based day counter
    int start_year;
    int start_month;
    int start_day;

    std::chrono::year_month_day toDate(int day_index) const;
    std::string getCurrentDateString() const;
    bool isWeekend(int day_index) const;
    bool isCurrentWeekend() const;
    int addBusinessDays(int from_day_index, int business_days) const;
    int addBusinessDaysFromNow(int business_days) const;
    void advanceDay();
    void setStartDate(int year, int month, int day);
};
```

### Key principles

1. **Single component, single entity**: all calendar state lives on the game entity; systems query it like any other component.
2. **Integer storage**: `current_day`, `start_year`, `start_month`, `start_day` are all plain `int`s. This keeps the component trivially copyable and destructible, so Flecs can `memcpy` it freely and serialize it via `flecs::meta` without lifecycle hooks. Adding a non-trivial member (e.g. `std::string`) would require registering `.ctor/.dtor/.copy/.move` hooks on the component.
3. **C++20 `<chrono>` internally**: `std::chrono::year_month_day` and `std::chrono::weekday` perform date arithmetic; this is a private implementation detail of the member functions.
4. **Business-day skip**: `addBusinessDays` advances day-by-day and skips Saturday/Sunday. Iterative, but game time scales (hundreds of days) make this negligible.
5. **Plugin-configurable**: Lua scripts call `calendar:setStartDate(year, month, day)` before the first update to set the scenario start date.

### What "weekend" means

Saturday and Sunday are non-working days. Holidays are out of scope for now; if needed they can be modelled as a separate component or Lua table later.

## Consequences

### Positive

- Date display requires no external library — standard C++20.
- Business-day durations ("20 working days") are now a single, testable function call.
- Start date is data, not code — scenario authors pick any date in Lua.
- Save/load is four integers; no migration complexity.

### Negative

- ~~Requires C++20 (`<chrono>` date types). The project already targets C++17 with `-std=c++17`; this ADR accepts bumping to C++20.~~ C++20 support is now available in all target platforms, so we can use `<chrono>` features without issue. Ref: https://github.com/pkuehne/solcorp/pull/71
- Business-day iteration is O(n) in the number of days added. Acceptable at game scales; would need a formula-based approach for very large jumps.
- No holiday support. Teams working across national boundaries would need an extension.

## Alternatives considered

**Stateless utility + separate `GameTime` component** — splits current day from start date into two components. Rejected: every calendar function call requires passing both pieces of data, the API is more awkward, and there is no serialization benefit.

**Store `std::chrono::year_month_day` directly** — not serializable by Flecs without custom reflection. Rejected.

**Lua-only calendar** — keeps all date logic in scripts. Rejected: C++ systems (construction, contracts) also need business-day calculations; duplicating the logic in two languages is worse.

## Example usage

```cpp
// Scenario setup (plugin or init system)
world.entity("Game").set<GameCalendar>({0, 1969, 7, 16});

// Advance time
world.system<GameCalendar>("AdvanceCalendar")
    .kind(UpdatePhase)
    .each([](GameCalendar& cal) { cal.advanceDay(); });

// Construction completion
int finish_day = cal.addBusinessDaysFromNow(20);

// Display
spdlog::info("Date: {}", cal.getCurrentDateString());
```

```lua
-- Lua plugin: Mars colony scenario
local cal = world:lookup("Game"):getGameCalendar()
cal:setStartDate(2045, 3, 1)
local done = cal:addBusinessDaysFromNow(30)
```
