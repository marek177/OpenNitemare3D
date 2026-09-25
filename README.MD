# OpenNitemare3D

C# reimplementation of the 1994 FPS **Nitemare 3-D**.

This repository is an older experimental implementation that is now being synchronized with the executable-level reverse-engineering work in `marek177/Nitemare3d-reversed`.

## 2026-09-25 synchronization

The newest executable-backed constants are now mirrored in [`OriginalRuntime.cs`](OriginalRuntime.cs): HUD/automap dispatchers and formulas, renderer globals/capacities, GUARD state/field corrections, IMG/UIF layout facts, and menu/save limits. The detailed evidence boundary lives in `marek177/Nitemare3d-reversed/docs/PROJECT_FINDINGS_DELTA_2026-09-25.md`.

The original C# `Player.RenderRaycaster` remains a provisional DDA renderer. Do not treat it as original NITE3W behavior; the recovered renderer is still the vector/span pipeline documented in the reverse-engineering repository.

## 2026 reverse-engineering update

Read [`REVERSE_ENGINEERING_SYNC_2026-09-22.md`](REVERSE_ENGINEERING_SYNC_2026-09-22.md) before changing original-game behavior.

Verified original-runtime/data constants have also been collected in [`OriginalRuntime.cs`](OriginalRuntime.cs). This separates evidence-backed values from port-only gameplay abstractions.

Important corrections to older assumptions:

- original Win16 OBJECT records are 28 bytes;
- original Win16 GUARD records are 26 bytes;
- visible wall spans are 20 bytes;
- normal GUARD creation initializes strength to 255;
- the original renderer is a projected wall-vector/span renderer, not Wolf3D one-grid-ray-per-column DDA;
- `WARP_L1..L4` are colored-key locked passage/wall classes;
- `LEVEL_UP2` is a level-skip gateway;
- original class spelling is `ONE_SHOT`.

The existing C# code does not yet implement all of those behaviors. Where current code differs, treat the sync document and the evidence in `Nitemare3d-reversed` as the fidelity target rather than assuming the old C# behavior is original behavior.

## Current verified MAP facts

MAP files are binary archives, not text maps.

For the supplied final game data:

- archive header: 514 bytes;
- map dimensions: 64x64 cells;
- cell format: wall byte + object byte;
- level payload: 8192 bytes;
- MAP.1 / MAP.2 / MAP.3 contain 11 / 10 / 10 payloads;
- world scale: 64 world units per tile;
- player starts use object IDs 1..4 for four cardinal orientations;
- spawn is at tile center (`x*64+32`, `y*64+32`);
- E1M11 is the internal/demo map.

The current `Level.LoadMap` implementation already uses the 514-byte header and 8192-byte payload; older README text calling those bytes unknown was outdated.

## Current implementation status

Implemented or partially implemented in this historical C# port:

- loading original bitmap/image resources;
- loading original maps;
- MIDI playback;
- Windows-version SFX playback;
- partial menu/UI;
- early sprites/enemies;
- partial curtains, warps and hidden panels;
- hand-authored wall/object mappings.

Still requiring major fidelity work:

- original vector/span renderer architecture;
- complete USE/door/panel/push/warp/special-wall dispatcher;
- original 28-byte OBJECT / 26-byte GUARD runtime model;
- 22-state guard dispatcher and strategy logic;
- original damage/difficulty transforms and two-phase Dracula behavior;
- complete enemy score/SFX mappings;
- DEMO playback/recording compatibility;
- USER.SAV/CONFIG.SAV compatibility;
- exact SND.DAT/VOC behavior;
- ENDING.FLI palette/delta correctness.

## How to compile

The project currently targets .NET 5 and uses SFML.NET plus MIDI libraries.

Open the folder in VS Code and use the existing launch/task configuration, or build with a compatible .NET SDK:

```text
dotnet build
```

The historical project may require compatible SFML native binaries on the host platform.

## Original game data

You need legally obtained original Nitemare 3-D data files. Do not commit proprietary game assets into this repository.

The old implementation was primarily developed against Episode 1 resources and Windows SND.DAT behavior. Future work should support the full episode/resource families:

- MAP.1..3
- IMG.1..3
- SND.DAT
- UIF.DAT
- GAME.PAL
- DEMO.1..3
- ENDING.FLI

`WALLS.*` and `OBJECTS.*` are useful definition/editor catalogs, but the original runtime behavior must be reconstructed from executable/data evidence rather than assuming those files are runtime script tables.

## DAT notes

Nitemare 3-D uses DAT-style containers for resources such as SND.DAT and UIF.DAT. Current reverse-engineering work has substantially improved the format and playback analysis since the original README was written.

Do not judge audio correctness only by whether an extracted file opens in a media player. SND/VOC/PCM interpretation should be validated against actual original in-game playback.

## Reverse-engineering source of truth

For implementation decisions, consult `marek177/Nitemare3d-reversed`, especially:

- `DISCOVERIES.md`
- `docs/ALL_THREADS_CONSOLIDATION_2026-09-22.md`
- `analysis/nite3w_renderer.md`
- `docs/GUARD_AI_RE.md`
- `docs/COMBAT_DAMAGE_RE.md`
- `docs/PLAYER_HEALTH_RE.md`
- `docs/PLAYER_COLLISION_RE.md`
- `docs/USE_INTERACTION_RE.md`
- `docs/DEMO_FORMAT_RE.md`
- `docs/SAVE_LIBRARIES_IDA_REPORT.md`

Evidence from the original executable/data should take precedence over guesses preserved in this older port.

## Historical upstream note

The original project README stated that a C rewrite was being developed and that this C# branch was incomplete. This fork keeps the C# code as a practical reference/porting target while progressively replacing guessed behavior with documented original behavior.
