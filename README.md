# OpenNitemare3D

**22–24 Oct 2026 implementation bridge:** [raw-byte snapshot/first-diff utilities, strict indexed+DAC acceptance gate and CTest](docs/OCT22_24_2026_IMPLEMENTATION_BRIDGE.md). These are generic byte-level tools, not yet a verified per-version USER.SAV decoder. [Research/evidence atlas](https://github.com/marek177/Nitemare3d-reversed/blob/main/docs/RE_CONSOLIDATION_20261022_24_SK.md).


Clean-room reconstruction of **Nitemare 3D**.

This repository was intentionally recreated from scratch. The previous OpenNitemare3D codebase is not used as the implementation base.

## Project goals

- Reconstruct gameplay and runtime behavior from the original Nitemare 3D executables and data files.
- Keep original game logic separate from platform backends.
- Target behavioral parity first, then runtime/pixel parity.
- Avoid temporary Wolf3D-style/DDA renderer code that can obscure the original Nitemare 3D rendering model.
- Document every reconstructed subsystem and the evidence behind it.

## Current status

This is the new clean bootstrap of the project.

Initial architecture:

- `core` — game/runtime orchestration
- `render` — Nitemare 3D renderer reconstruction boundary
- future: `world`, `objects`, `guards`, `combat`, `audio`, `input`, `save`, `platform`

The renderer is intentionally only an interface/stub at this stage. No legacy DDA renderer has been carried over.

## Build

Requirements:

- CMake 3.20+
- C++20 compiler

Example:

```bash
cmake -S . -B build
cmake --build build
```

## Reverse-engineering priorities

1. Wall traversal / renderer projection and clipping
2. OBJECT runtime and scheduler
3. GUARD movement, AI and combat states
4. Collision / radius / interaction rules
5. Global tick dispatch and animation state transitions
6. USER.SAV serialization/runtime behavior
7. BSF remaining runtime semantics

## Legal / data note

Original commercial game assets and executables are not distributed by this repository.
