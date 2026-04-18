# UI & Widgets

## ImGui Integration

All UI is built with Dear ImGui, called during `GuiPhase`. The SDL2 renderer pipeline handles ImGui draw data in `RenderPhase`.

UI code lives in `src/modules/engine/gui.cpp`, which calls into per-module window structs.

## Window Pattern

Each UI window is a struct with a `show()` method:

```cpp
// In module header
struct MySiteWindow {
    void show(flecs::world& world);
};

// In gui.cpp
my_site_window.show(world);
```

The `show()` method calls `ImGui::Begin` / `ImGui::End` and queries the ECS for display data.

## Widgets

Reusable ImGui components live in `src/widgets/`. Use these rather than duplicating ImGui patterns across modules.

### ActionButton

Many actions in the game require validation before they can be executed. The `ActionButton` widget encapsulates this pattern, taking a label, a tooltip and a list of validation issues. It renders a disabled button with a tooltip if there are any issues, and an enabled button that executes the action if there are none.

### NotImplementedPopup
For features that are not yet implemented, the `NotImplementedPopup` popup provides a consistent way to inform users that the feature is coming soon. It can be triggered from any UI element to display a modal popup with a message.

## Testing UI Logic

ImGui rendering is not unit-testable, but logic embedded in draw functions **should be extracted** into free functions, declared in the header, and tested with Catch2. This pattern has caught real bugs. See the [testing guidance in CLAUDE.md](../../CLAUDE.md).
