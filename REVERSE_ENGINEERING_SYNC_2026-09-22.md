# OpenNitemare3D reverse-engineering synchronization

Date: 2026-09-22

This fork predates the current executable-level reverse-engineering work. The original C# code is useful as an experimental reimplementation, but many names, constants and behaviors were originally guessed from gameplay or hand-authored tables.

The current evidence-backed source of truth is `marek177/Nitemare3d-reversed`. This document records what should now guide future OpenNitemare3D corrections.

## Do not copy old assumptions forward

The following old approximations are superseded for NITE3W.EXE V1.10:

- OBJECT is 28 bytes, not ~80 bytes.
- GUARD is 26 bytes, not ~98 bytes.
- visible wall span is 20 bytes, not ~52 bytes.
- the renderer is not Wolf3D-style one-grid-ray-per-column DDA.
- normal GUARD strength is initialized to 255; no class-specific post-spawn HP table has been found in the original Win16 executable.
- `WARP_L1..L4` are colored-key locked wall/passage classes, not generic teleport classes.
- `LEVEL_UP2` is a level-skip gateway.
- original definition spelling is `ONE_SHOT`.

## Verified map/world model

- map dimensions: 64x64;
- two bytes per cell: wall byte then object byte;
- level payload: 8192 bytes;
- supplied archive header: 514 bytes;
- world scale: 64 world units per tile;
- player spawn: tile center `x*64+32, y*64+32`;
- player start object IDs 1..4 encode four cardinal orientations;
- E1M11 is the internal/demo map;
- E1M3/E1M11 are a positive-control pair for demo matching.

The current `Level.LoadMap` implementation already relies on the 514-byte header and 8192-byte payload, but README text previously described MAP as a text format and called the header unknown. That documentation is obsolete.

## Verified runtime capacities from NITE3W.EXE V1.10

| Family | Capacity | Record size |
|---|---:|---:|
| doors | 64 | 22 B |
| panels | 32 | 22 B |
| pushes | 12 | 6 B |
| objects | 350 | 28 B |
| guards | 100 | 26 B |
| vectors | 1000 | 28 B |
| VECLIST/orientation | 333 | 4-byte far pointer |
| visible wall spans | 50 | 20 B |
| projected sprite commands | 100 | 18 B |

These are original-runtime facts, not arbitrary port limits.

## Player

Verified original Windows behavior:

- player health runtime byte is clamped to a normal 0..100 range;
- new game/reset uses 100 health;
- lethal damage writes exactly zero before the death transition;
- player collision half extent is 27 world units;
- movement collision handles X/Y separately and therefore slides along walls;
- difficulty numeric ordering is easier / baseline / harder.

Recovered input bits:

- `0x0002` forward;
- `0x0004` backward;
- `0x0008` / `0x0010` turn directions;
- `0x0020` double movement/turn increment;
- `0x0040` force movement/turn increment to 1;
- `0x0080` fire;
- `0x0100` strafe modifier;
- `0x0200` edge-triggered use/action.

## Renderer architecture

The original Win16 renderer builds wall vectors from exposed MAP boundaries and projects those segments. High-level recovered pipeline:

```text
MAP 64x64
 -> exposed boundary extraction + merging
 -> VEC[1000], 28 B each
 -> four orientation-specific VECLIST[333]
 -> transform / clip / project
 -> 320-column wall owner table
 -> <=50 visible wall spans
 -> wall rasterization + per-column occlusion
 -> <=100 projected sprite commands
 -> 320x200 indexed framebuffer
```

Normal 3-D viewport is 304x152 at x=8..311, y=4..155 with center `(160,80)`.

A fidelity-oriented rewrite of OpenNitemare3D should eventually replace tile/raycast assumptions with this recovered vector/span model rather than attempting to tune a Wolf3D-style renderer until it merely looks similar.

## USE / doors / wall semantics

USE is an edge-triggered interaction that resolves an adjacent cardinal cell. It is a dispatcher entry point for more than ordinary doors.

Important original families:

- `WARP_L1..L4`: red/green/blue/yellow key-locked passages;
- `WARP_1..8`: paired stair/dumbwaiter/vertical connections;
- `WARP_E1/E2`: elevator groups;
- `WARP_S1/S2`: mirror-related special warps;
- normal/locked/remote/curtain door orientations;
- `CONTROL`;
- `LEVEL_UP`, `LEVEL_UP2`;
- `WALL_EX1`, `WALL_EX2`;
- `ONE_SHOT`;
- `SPECIAL1`;
- `ACTIONSPOT`, `TRIGGER1/2`;
- `RETREAT`, `TURN`, `FLEE`;
- `SAFESPOT`.

Known safe-combination values in the original definition/data material include `333`, `01532`, `080993`, `372535`.

## GUARD model

Original Windows runtime:

- GUARD record size 26 bytes, max 100;
- linked OBJECT record size 28 bytes;
- OBJECT `+06` is class;
- OBJECT `+10/+12` are world X/Y;
- GUARD `+08` links to the OBJECT slot;
- GUARD `+0A` strategy;
- GUARD `+0B` state;
- GUARD `+0C` nextstate;
- GUARD `+06` timer;
- GUARD `+10` strength;
- GUARD `+11` octant;
- GUARD `+12` result octant.

The dispatcher uses 22 states (`0x00..0x15`). State `0x15` is the confirmed pain/hit reaction. Do not force the current simplified `idle/chasing/attacking/dead/patrol/roar` enum onto the original state IDs; it is a port abstraction, not a recovered original enum.

Normal GUARD creation initializes strength to 255. The current C# `Guard` field initialized to 100 should therefore not be treated as an original-game fact.

## Enemy class / score findings

Recovered score switch:

| Class | Role | Score |
|---:|---|---:|
| 0x08 | Bat | 25 |
| 0x09 | Frankenstein | 75 |
| 0x0A | Mummy | 50 |
| 0x0B | Skeleton | 100 |
| 0x0C | Mrs H. | 250 |
| 0x0D | Zelda | 150 |
| 0x0E | Vampira | 200 |
| 0x0F | Baddie #1 | 100 |
| 0x10 | Baddie #2 | 100 |
| 0x11 | Dracula phase 1 | 0 |
| 0x12 | Cemetery Gargoyle | 150 |
| 0x13 | Garden Gargoyle | 150 |
| 0x14 | Dracula-Bat phase 2 / GUARD13 | 200 |
| 0x15 | Penelope | -1000 |
| 0x16 | Dr. Hamerstein | 1000 |
| 0x17 | Tall slim robot | 100 |
| 0x18 | Trashcan robot | 200 |
| 0x19 | Cannon | 0 |
| 0x1A | Ghost | 25 |
| 0x1B | Goldie | 100 |
| 0x1C | Greenie | 100 |
| 0x1D | Demon | 250 |
| 0x1E | Alien #1 | 250 |
| 0x1F | Alien #2 | 200 |
| 0x20 | unresolved/cut/fallback GUARD25 | 50 |

Dancers/GUARD26 are outside this switch and use separate scripted behavior.

### Dracula two-phase behavior

On lethal phase-1 damage, class `0x11` transforms in place to `0x14`, GUARD strength is restored to 255 and state/timer/sequence fields are reset. Therefore GUARD13 is the internal Dracula-Bat second phase, not another ordinary Bat.

## Combat / weapon model

Original weapon selector:

- 0 Single Shot Laser;
- 1 Magic Wand;
- 2 Silver Pistol;
- 3 Continuous Laser;
- 0xFF none/unset.

Ammo pools:

- silver byte `0x4C1F`;
- laser byte `0x4C20` shared by single/continuous laser;
- wand byte `0x4C44`.

Normal pickup/display cap is 100. Silver/laser use signed comparisons in audited paths, so manually edited 128..255 values have signed-byte quirks.

Player->GUARD damage is not a fixed per-weapon integer. The recovered producer begins with projected/view geometry plus `random()%25`, then applies class/weapon resistance and difficulty. Consequently a universal fixed shots-to-kill table is incorrect unless distance/projection, RNG, weapon and difficulty are fixed.

Important special cases:

- Ghost: Wand reaches it in the audited damage path; other weapons return zero there.
- Alien #1/#2: Wand returns zero there.
- Penelope/Cannon: zero normal weapon damage in that producer.
- Hamerstein: special gated damage branch.
- weapon jam is scripted by E1M9 events, not a recovered random probability.

## DEMO

Recovered packed record:

```text
byte  eventByte
word  inputMask
byte  pad
dword timestamp
```

The supplied files have a 6-byte header representing WORDs `(10,5,20)`.

DEMO.1 is associated with attract-mode E1M11. DEMO.2/3 level origins remain unresolved and must not be hard-coded from filename suffixes.

## Saves / config

Win16 `CONFIG.SAV` is exactly 20 bytes and contains display/control/audio enable/volume values plus four cheat flags.

Win16 `USER.SAV` uses fixed 55015-byte (`0xD6E7`) records and stores an 8192-byte mutable MAP snapshot plus runtime arrays/state. This format must be implemented from the recovered layout rather than guessed object serialization if save compatibility becomes a goal.

## Audio / FLI

SND.DAT extraction must be checked against actual original in-game playback. A VOC that merely opens is not proof that rate/chunk interpretation is correct.

ENDING.FLI currently audits as 488 frames using `COLOR_64`, `BRUN`, `LC`, `BLACK`, `COPY`; palette and delta-frame dependencies must be preserved during editing.

## Recommended OpenNitemare3D implementation order

1. Centralize verified constants and distinguish them from port-only abstractions.
2. Correct MAP/world documentation and map indexing assumptions.
3. Rework interaction semantics around the recovered USE dispatcher.
4. Expand wall metadata from texture-only cases to class/flags/handler semantics.
5. Replace guessed GUARD HP/state assumptions with the recovered OBJECT/GUARD model.
6. Add class/score mapping and Dracula two-phase behavior.
7. Rebuild damage/difficulty behavior from the original transforms.
8. Rework renderer architecture toward vector/span projection.
9. Implement demo records exactly and use them as regression tests.
10. Add save/audio compatibility only from verified layouts/formats.

## Source documents

See `marek177/Nitemare3d-reversed`:

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

Only facts backed by the original executable/data/runtime should be labeled original behavior in this fork.