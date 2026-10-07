# Nitemare 3D RE — Weekly Summary of New Findings

Executive and technical overview of new discoveries.

**Period:** October 1–7, 2026

## Confidence legend

**CONFIRMED** — directly supported by disassembly/dumps or agreement between multiple artifacts.  
**STRONG** — very strongly supported statically, but a targeted runtime proof is still missing.  
**CORRECTION** — a new finding invalidates or refines an older hypothesis.  
**OPEN** — behavior/order is not yet closed by a 1:1 runtime test.

# 1. What changed most during the week

DOS v2.0 moved from “most blocks are known” to a state where the canonical unpacked image and practically all Nitemare-owned static code are mapped. What remains is mainly the exact boundary of several unusual entry/fall-through points plus behavioral runtime proof.

The `codes.zip` and `_reko.zip` archives produced several important address corrections and removed older incorrect assumptions. The most important correction is that DOS v2.0 player X/Y are not the old `3:4BF6/4BF8`; they are `DS:4162/4164`. Offsets must therefore never be transferred between builds without executable fingerprinting.

A concrete projectile/effect pool model was closed statically: 8 slots × `0x2A` bytes, lifecycle `0 → 1 → 2 → 0`, a spawn/update/render chain, and DDA/Bresenham-like movement fields.

Player collision was refined into a step-based resolver with a `0x1B` (27 world-unit) footprint, a pair of neighboring cells, and concrete hard-block/dynamic/side-effect flag semantics.

The “secret door” is no longer only a hypothesis. A 14-byte runtime record and a handler were identified that retract up to four VEC segments by 2 world units per update. This is not a rigid pushwall.

GUARD AI now has a concrete pool/record layout, state dispatcher, known RNG consumption in states `03/05/06/13`, planner timers, and difficulty scaling.

The renderer now has a concrete span-buffer structure, OWNER/VEC/shade buffers, framebuffer/palette sizes, and breakpoint anchors for pixel-perfect first-divergence testing.

The RNG model and ordering were substantially refined. DOS v2.0 contains at least 20 direct RNG callsites; audio, AI, damage and animation share the same stream, so an apparently harmless branch can desynchronize DEMO playback.

# 2. Most important confirmed data

| Area | New finding | Status |
| --- | --- | --- |
| Canonical DOS v2.0 image | 171,360 B; SHA-256 `e2efde70af9637fb233bcf4a8cb1cec736ffd81fa90998cc47834b3f54f1f297` | CONFIRMED |
| Player X/Y (DOS v2.0) | `DS:4162 / DS:4164`; commit `0800:68B5 / 68B9` | CONFIRMED |
| Player movement | `0800:6914 → 6488 → 688C`; step resolver `6378`; footprint `0x1B` | CONFIRMED |
| Player HP / damage | HP `DS:4189`; damage handler `0800:6A70` | CONFIRMED |
| Weapon/ammo | weapon `DS:418F`; ammo `DS:418B`, `418C`, `41B0`; gate `8F12` | CONFIRMED |
| Projectile/effect pool | `DS:41B6`; `8 × 0x2A = 0x150 B`; scheduler `8230`, movement `8142` | CONFIRMED |
| GUARD pool, DOS | base `DS:264E`; stride `0x1A`; count `[6276]`; loop `5F26` | STRONG / statically confirmed |
| Secret panel pool | base `34F6`; record `0x0E B`; VEC0–3, MAP cell, state; handler `0AEA` | CONFIRMED |
| Renderer SPAN | count `DS:4D64`; base `DS:4D6E`; stride `0x12`; max 50; 910 B | CONFIRMED |
| Frame/palette | `320×200×8-bit = 64,000 B`; palette 768 B | CONFIRMED |
| RNG | `state = state*214013 + 2531011 (mod 2^32)`; `result=(state>>16)&0x7FFF` | CONFIRMED |

# 3. Corrections to older assumptions

| Older assumption | New finding | RE consequence |
| --- | --- | --- |
| `3:4BF6 / 3:4BF8` are player X/Y | DOS v2.0 player X/Y = `DS:4162/4164`; the older offsets belong to another build/context. | All breakpoint/watchpoint profiles must be build-specific. |
| Repeated `+0x50` proves an OBJECT stride | Sites `0800:1A0E,1A23,1B0D,1CF7,1DC6,1F8E` are VGA/planar scanline stride. | The old evidence for a `0x50` OBJECT stride is rejected. |
| `FUN_1000_70D6` = OBJECT scheduler | `70D6` is more likely an input/control/event dispatcher. | Search/validate the OBJECT scheduler elsewhere; do not mix input with object ticking. |
| Secret door = classic pushwall | Record `34F6` + handler `0AEA` retract individual VEC segments. | Implement segmented geometry, not rigid whole-wall translation. |
| RNG is used only through a known wrapper | v20 has 20 direct callsites to `0FBA:01A0`. | RNG census must include direct far calls or DEMO ordering will diverge. |

# 4. New behavioral discoveries

Weapon cadence is consistent across DOS builds: thresholds `[2,1,3,1]`, with a separate held-FIRE exception for weapon 3. A failed fire attempt still consumes a cadence step.

If the projectile pool is full, a new projectile is not allocated; static analysis indicates that ammo is not consumed in this branch.

The hitscan branch can hit multiple GUARDs in one action; damage/pain/death and alert branches have their own RNG consumption.

GUARD state 06, when `blockedX && blockedY`, consumes one RNG draw; bit 0 selects whether stored X or Y direction is flipped.

Planner `55A8` uses timers `RNG%8+8` (8–15), with difficulty effectively scaling the range to Easy 16–30, Medium 8–15, Hard 4–7.

Automap and enemy detector have separate energy pools and different drain intervals: automap every 16 ticks, detector every 8 ticks; both switch off when their energy reaches zero.

DEMO compatibility is asymmetric: Windows can play the DOS demo, while DOS interpreting a Windows demo still understands some events (weapon switching/map toggle) but player movement may not match. The difference is therefore likely in input/event encoding or cadence.

# 5. Resource/data formats and lineage

`SND.DAT` and `UIF.DAT` use a 6-byte index record: `WORD size + DWORD offset`. UIF is not only a PCX archive; it also contains fonts and UI bitmaps.

`NITE3D.BSF` uses the XOR key `Copyright 1992, David P Gray, Gray Design Associates`. For DOS v1.9, a 54-byte header/checksum/decode context was identified.

Different `NITE3D.BSF` variants of roughly 10,250 to 15,800 bytes with different CRCs were observed, so build fingerprinting is also required for BSF data.

Hugo/ScummVM is useful as lineage/reference material for the state/timer philosophy, but N3D GUARD/projectile/door/raycaster logic must not be copied 1:1 from it. OpenNitemare3D also does not yet contain a full `NITE3D.BSF` loader/parser.

# 6. Status at the end of the period

> The percentages below are working RE estimates, not automatically measured coverage metrics. For a 1:1 target, the decisive proof is a first-divergence runtime test.

| Area | Working status | What still prevents 100% |
| --- | ---: | --- |
| Static core / code ownership | ~98–100% | several unusual boundary/secondary-entry points; build pairing |
| Player collision | ~93–96% | sliding and edge ordering on cell/object boundaries |
| Projectiles | ~90–94% | exact collision ordering, stale render-cache edge case, timing |
| Doors/special walls | ~94–97% | occupancy and all transition edge cases |
| Secret panel runtime | ~86–90% | exact USE trigger/collision during movement and timing |
| GUARD runtime/AI | ~91–95% | LOS edges, blocked reactions, boss/class-specific branches |
| Renderer architecture | ~95–98% | pixel-perfect rounding/clipping/draw-order parity |
| Pixel-perfect renderer | ~79–85% | framebuffer first divergence, exact fixed-point and clipping |
| DEMO determinism | ~83–89% | RNG ordering + scheduler timing + cross-version event encoding |

# 7. Main conclusion

The most important progress this week is not merely a higher static-knowledge percentage. The project moved toward concrete build-specific data, exact record layouts, RNG ordering, and testable first-divergence anchors. The work is increasingly less about “finding functions” and increasingly about closing behavioral, timing, and pixel-level parity.
