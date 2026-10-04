# Architecture

## Principle

OpenNitemare3D is rebuilt around reconstructed Nitemare 3D behavior rather than around the deleted legacy implementation.

The codebase is split into three layers:

1. **Game logic** — rules and state that should remain platform independent.
2. **Original-runtime reconstruction** — subsystems whose behavior is derived from DOS/Win16 binaries and game data.
3. **Platform backend** — modern Windows/input/audio/video/file-system integration.

## Planned modules

- `core`
  - main loop
  - tick scheduling
  - state transitions
- `world`
  - map loading
  - wall metadata
  - triggers / specials / teleport / doors
- `objects`
  - OBJECT records
  - scheduler
  - movement and interaction
- `guards`
  - GUARD records
  - movement
  - state machine
  - combat decisions
- `combat`
  - weapons
  - damage
  - projectiles
  - pain/death
- `render`
  - original wall traversal model
  - projection
  - clipping
  - sprite/object projection
  - final framebuffer
- `audio`
  - SND.DAT
  - music/SFX dispatch
- `save`
  - USER.SAV serialization
- `platform`
  - window
  - input
  - timing
  - modern audio/video output

## Renderer rule

No legacy DDA renderer is retained as an architectural dependency.

Temporary visualization code may only be added when it is clearly isolated from the parity renderer and cannot be confused with reconstructed original behavior.

## Parity levels

Every reconstructed subsystem should eventually track:

- static understanding
- runtime behavioral parity
- data-format parity
- timing parity
- pixel/audio parity where applicable
