# Reusable material from the deleted implementation

The previous OpenNitemare3D implementation is **not** restored as the engine base.

A review of the archived C# tree found a small set of material that is useful to retain because it records reverse-engineering facts rather than approximated engine behavior.

## Imported as reference facts

The following were translated to C++ reference-only headers:

- `src/re/OriginalRuntime.h`
  - map/world dimensions
  - input masks
  - recovered capacities and record strides
  - viewport/framebuffer facts
  - HUD/automap facts
  - GUARD states and offsets
  - renderer-related Win16 globals
  - IMG/UIF facts
  - save/demo facts
  - score table
- `src/re/RecoveredMechanics.h`
  - recovered wall/object flags
  - OBJECT/GUARD field offsets
  - small invariant helpers
- `src/re/RecoveredInteractionFacts.h`
  - remote-door classes/commands
  - script-touch flag
  - portal and exploding-wall facts

These headers intentionally contain **no scheduler, AI, movement, renderer or gameplay loop implementation**.

## Not imported as authoritative runtime code

The old implementations of these systems remain excluded:

- raycasting / renderer
- GUARD AI
- projectile behavior
- player movement
- collision
- entity scheduler
- animation timing
- level runtime
- SFML/game-loop architecture

They contain approximations and would interfere with the goal of reconstructing original Nitemare 3D behavior.

## Old enum tables

The archived `ObjectType.cs` and `WallType.cs` are useful as naming/reference tables, but they should be imported only after cross-checking IDs against the original episode data and executable behavior. They are therefore not yet treated as authoritative runtime definitions.

## Rule

Nothing copied from the old repository becomes an original-game fact merely because it existed in the old source tree. Runtime code must be supported by current reverse-engineering evidence.
