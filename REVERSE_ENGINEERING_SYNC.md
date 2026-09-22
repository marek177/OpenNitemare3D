# OpenNitemare3D reverse-engineering synchronization

Date: 2026-09-22

This branch is synchronized from the clean-room reverse-engineering repository:

- `marek177/Nitemare3d-reversed`
- canonical cross-thread entry point: `docs/ALL_THREADS_CONSOLIDATION_2026-09-22.md`

Only values backed by original NITE3W.EXE/data/save evidence are promoted into `RecoveredRuntime.cs`. Unknown numeric values are not invented.

## Corrections already carried into this branch

- MAP geometry is 64×64, 2 bytes per cell, 8192 bytes per level, with a 514-byte archive header.
- Supplied MAP payload counts are E1=11, E2=10, E3=10; E1M11 is the internal/demo map.
- World scale is 64 units per tile.
- Player collision half-extent is 27 world units.
- Movement is subdivided into small original-world-unit steps and resolves X/Y separately for wall sliding.
- USE/ACTION is edge-triggered and targets one adjacent cardinal cell.
- Fresh GUARD strength is `0xFF` / 255.
- Original runtime OBJECT records are 28 bytes, not the older guessed 80 bytes.
- Original runtime GUARD records are 26 bytes, not the older guessed 98 bytes.
- Door/panel/push/vector/span/sprite capacities and record sizes are now recorded in `RecoveredRuntime.cs`.
- Difficulty value ordering is constrained by player->guard damage, guard->player damage and GUARD timer scaling.
- Per-GUARD score values recovered from the original score switch are recorded, including Dracula=0, Demon=250, Dr. Hamerstein=1000 and Penelope=-1000.

## Original renderer facts now preserved in source constants

The original Windows renderer is not a Wolf3D one-ray-per-screen-column DDA implementation.

Recovered architecture:

```text
64x64 MAP
 -> boundary extraction / merge
 -> 1000 x 28-byte VEC pool
 -> 4 x VECLIST[333] far-pointer indexes
 -> project / near-clip vectors
 -> 320-entry far-pointer owner table
 -> <=50 wall spans, 20 bytes each
 -> wall-column draw + 320-entry wall-occlusion table
 -> <=100 projected sprite commands, 18 bytes each
 -> 320x200 indexed framebuffer
```

Normal 3-D viewport:

- x=8..311
- y=4..155
- 304×152
- center=(160,80)

Important original data-segment anchors preserved in `RecoveredRuntime.cs`:

- owner table `0x53FE`;
- wall occlusion/silhouette table `0x58FE`;
- wall-span count `0x5E7E`, spans `0x5E88`;
- projected sprite queue `0x6270`;
- VECLIST counts `0x697A/7C/7E/80`;
- VECLIST bases `0x6982/0x6EB6/0x73EA/0x791E`.

These constants document the original engine and allow later renderer replacement/reconstruction work without losing the recovered layout.

## Guard / combat synchronization

Confirmed original GUARD facts now represented or referenced by this branch:

- capacity 100;
- record stride 26 bytes;
- fresh strength/HP 255;
- 22-state dispatcher covering states `0x00..0x15`;
- state `0x15` is the confirmed pain/hit reaction state;
- score switch values for GUARD1..25, with GUARD26 following the default zero-score path.

Exact enemy-specific initial HP values must **not** be invented: direct-write audit supports common fresh strength 255, while class/weapon toughness is largely implemented in the damage transforms.

The original player->GUARD damage matrix is documented in `Nitemare3d-reversed/docs/COMBAT_DAMAGE_RE.md` and should be ported only where the visible enemy/object class mapping is verified.

Difficulty value behavior:

| value | player -> enemy | enemy -> player | guard timing |
|---:|---:|---:|---:|
| 0 | x2 | /2 | slower |
| 1 | x1 | x1 | baseline |
| 2 | /2 | x2 | faster |

## Resource synchronization notes

- Windows SND.DAT directory: 160 × 6-byte entries (`uint16 length + uint32 offset`).
- SFX playback in the audited Windows build: raw PCM, mono, 11025 Hz, 8-bit.
- IDs 1..15 are MIDI; IDs 34..110 are logical SFX range; 111 is the zero-length end sentinel.
- GAME.PAL provides the original indexed palette; recovered default floor/ceiling indexes are `0x0C` / `0x11`.
- Sprite transparency in the audited original WinG sprite path uses palette index `0x29`.
- ENDING.FLI is 320×200 with 488 frames in the header; decoders may expose a 489th ring/repeat frame.

## Open items deliberately not guessed

- final semantic names for all GUARD states 02..14;
- guard movement speed and exact attack cadence;
- complete enemy-to-player class/name table;
- projectile speed/lifetime/hit radius/friendly-fire;
- complete wall class numeric dispatcher;
- exact door/panel 22-byte layouts;
- animated-wall timer/sequence semantics;
- exact `REVWALL`, `ONE_SHOT`, `SPECIAL1`, mirror and remote-control handlers;
- exact owner-conflict math in the original renderer;
- complete texture-U/special-wall logic;
- full DOS/VGA fallback renderer path;
- unresolved USER.SAV runtime blocks;
- full UIF/SND semantic ID mapping.

The rule for future synchronization remains: **verified values may enter runtime code; partial values stay documented until their exact executable/data path is proven.**
