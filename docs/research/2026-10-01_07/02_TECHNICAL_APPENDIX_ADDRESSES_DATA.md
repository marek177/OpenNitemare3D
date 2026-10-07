# Nitemare 3D RE — Technical Appendix

Addresses, record layouts, global fields and algorithms.

**Period:** October 1–7, 2026

## Confidence legend

**CONFIRMED** — directly supported by disassembly/dumps or agreement between multiple artifacts.  
**STRONG** — very strongly supported statically, but a targeted runtime proof is still missing.  
**CORRECTION** — a new finding invalidates or refines an older hypothesis.  
**OPEN** — behavior/order is not yet closed by a 1:1 runtime test.

# 1. Build and artifact identity

| Artifact | Finding | Note |
| --- | --- | --- |
| DOS v2.0 canonical unpacked image | 171,360 B; SHA-256 `e2efde70af9637fb233bcf4a8cb1cec736ffd81fa90998cc47834b3f54f1f297` | Baseline for the build-specific map. |
| `codes.zip` | 17 dumps, about 332k lines; after deduplication 8 unique DOS + 4 Win16 variants | v18 == v19; several Win16 duplicates. |
| `_reko.zip` | 415 files/projects; `.asm/.dis/.c/.h`, loader/relocation and Win16 structure recovery | N3D-UNFU and codes/v20 agree on 46,880 B of analyzed content. |
| Address portability | `6EC8`, `9806`, `7E5E`, `4BF6` and others cannot be transferred between builds | Use fingerprint + per-build symbol map. |

# 2. DOS v2.0 — player, input and combat

| Symbol/area | Address / field | Meaning / behavior |
| --- | --- | --- |
| Player X | `DS:4162` | world X |
| Player Y | `DS:4164` | world Y |
| Movement entry | `0800:6914` | entry to movement chain |
| Step movement | `0800:6488` | iterates movement in steps; footprint `0x1B` |
| Step collision resolver | `0800:6378` | tests two footprint cells; returns requested step or `AX=0` |
| Commit/bookkeeping | `0800:688C` | `6636/676E →` write X/Y `→ SAR ...,6` map-cell update |
| Commit X/Y | `0800:68B5 / 68B9` | concrete store sites |
| Player HP | `DS:4189` | maximum 100 in gameplay model |
| Player damage | `0800:6A70` | damage/death handler |
| Current weapon | `DS:418F` | weapon selector |
| Ammo fields | `DS:418B / 418C / 41B0` | ammo pools |
| Action dispatcher | `0800:89A2` | FIRE/USE and action gating |
| Fire gate | `0800:8F12` | cadence/ammo/fire branch |
| Ammo decrement | `0800:8F42 / 8F54 / 8F66` | weapon-specific decrements |
| Input bitfield | `DS:4308` | current input mask |
| Input toggle/state | `DS:4309` | toggle/edge state |
| Input handlers | `0800:9646 / 976F / 977F / 9788` | key/input event processing |
| Keyboard ISR | `0800:6B00` | low-level keyboard path |

# 3. Collision flag semantics

| Source | Bit | New meaning |
| --- | --- | --- |
| `D00C` primary cell | `0x04` | immediate hard block |
| `D00C` primary cell | `0x08` | conditional dynamic geometry/passability |
| `D00C` primary cell | `0x40` | side effect/trigger without direct blocking |
| `D10C` secondary cell | `0x04` | callback/side effect is invoked before the next block test |
| `D10C` secondary cell | `0x02` | blocks movement |

**CORRECTION:** Movement is not a simple endpoint X-first/Y-first test. `6488` performs repeated step checks and axis order depends on direction/state.

# 4. Projectile/effect runtime pool (DOS v2.0)

| Offset | Size | Interpretation |
| --- | --- | --- |
| `+00` | WORD | major/dominant axis |
| `+02` | WORD | error accumulator |
| `+04` | WORD | increment |
| `+06` | WORD | correction |
| `+08` | WORD | X step |
| `+0A` | WORD | Y step |
| `+0C` | ? | lifecycle state: 0 free, 1 active, 2 impact/terminal |
| `+0E` | 28 B | embedded OBJECT record |
| `+11` | BYTE | frame |
| `+12` | BYTE | animation/type |
| `+16/+18` | DWORD | deadline / next animation tick |
| `+1E` | WORD | world X |
| `+20` | WORD | world Y |

| Routine | Address | Role |
| --- | --- | --- |
| Allocator/spawn | `0800:7EDE` | first-free from 8 slots |
| Secondary spawn/init | `0800:7F96` | additional initialization |
| Movement/collision | `0800:8142` | movement integration, hit/collision, state `1→2` |
| Scheduler | `0800:8230` | iterates 8 slots |
| Render/projection | `0800:B2D4` | projection/entity submit |

Pool base `DS:41B6`, 8 slots × `0x2A B = 0x150 B`; approximate range `41B6..42DC`.

Collision/hit tolerance against GUARD position is `abs(dx)<10` and `abs(dy)<10`, effectively ±9.

On collision, an active projectile changes to state 2, frame is reset and flag `0x10` is set; the one-shot animation later finishes in state 0.

Projectile update occurs before input/FIRE in scheduler order, so a newly allocated projectile starts moving only on the following frame/tick.

OBJECT `+18/+19` contains projected-Y/baseline cache; static analysis indicates it can survive slot reuse and participate in projectile damage. This is an important runtime edge case to verify.

# 5. Weapon cadence and FIRE pipeline

| Element | Value |
| --- | --- |
| Cadence counter | `DS:1442` |
| Threshold table | `DS:143E` |
| Selector | `DS:418F` |
| Thresholds | `[2, 1, 3, 1]` |
| Cadence routine | around `0800:9026` |
| Weapon 3 | held-FIRE exception |
| Failed fire | consumes a cadence step |

**STRONG:** The cadence routine was paired across DOS builds and has a consistent 42-instruction shape. Exact addresses outside v2.0 must be mapped per build.

# 6. GUARD runtime

| Platform/build | Base | Stride | Count/max | Note |
| --- | --- | --- | --- | --- |
| DOS v2.0 | `DS:264E` | `0x1A / 26 B` | count `[6276]` | loop `0800:5F26` |
| Analyzed Win16 build | `0195:93AE` | `0x1A / 26 B` | max 100; count `[7E5E]` | build-specific address |

| GUARD offset | Meaning |
| --- | --- |
| `+06` | timer |
| `+08` | descriptor index |
| `+0A` | strategy |
| `+0B` | state |
| `+0C` | nextstate |
| `+0D` | o_id |
| `+10` | strength/HP-like field |
| `+11` | octant |
| `+12` | resoct |
| `+13/+14` | position-related fields |
| `+16` | flag |

Win16 `InitGuard 00C7:B02C` defaults to `strategy=0`, `state=7`, `nextstate=2`, field `+16=1`; class-specific override branches are applied afterward.

Descriptor table stride is `0x1C`.

DOS v2.0 state dispatcher `0800:59F0` handles states `00–15`; state 03 perception leads into attack/planner logic, state 05 chain ends in `55A8`, state 06 handles blocked movement, and state 13 uses a longer random timer.

`55A8`: timers `RNG%8+8` (8–15); difficulty effectively scales intervals to Easy 16–30, Medium 8–15, Hard 4–7.

`58A0/58A7`: state `0x13` with timer `RNG%80+8 = 8–87`.

`5092/525E`: when `blockedX && blockedY`, exactly one RNG draw is consumed; bit0=1 flips X, bit0=0 flips Y; no second movement commit is performed.

# 7. RNG — exact model and callsites

```text
state_next = (state * 214013 + 2531011) mod 2^32
result     = (state_next >> 16) & 0x7FFF
```

The DEMO seed is 1 in the analyzed scenario.

Direct runtime target in DOS v2.0: `0FBA:01A0`. A wrapper-only XREF census is not sufficient.

20 direct callsites identified in v20:

```text
2478, 525E, 5582, 5666, 56A6, 573F, 58A7, 85AD, 887C, 9A68,
9A77, 9C98, 9CC2, 9D6E, 9F0C, 9F36, 9FE3, A047, A12B, B276
```

`241E` selects an animation variant using `RNG&7` with a rejection loop, so one visual animation can consume `0..N` draws and shift the entire shared RNG stream.

# 8. Secret panel / special wall

| Offset in 14-byte record | Meaning |
| --- | --- |
| `+00/+02/+04/+06` | VEC0–VEC3 offsets |
| `+08` | MAP cell offset |
| `+0A` | MAP cell segment |
| `+0C` | runtime state |

Pool base `34F6`; record `0x0E B`; the working model assumes 32 records.

USE class 3 (around `0867`) activates the panel: `state=2`, SFX `0x27`, and initial ±1 adjustment of the four VEC offsets.

Handler `0AEA` retracts the first endpoint of each VEC toward the second by 2 world units per update.

At completion, `VEC.flags bit0`, the map cell and `record.state=0` are cleared/reset.

This behavior is segmented geometry retraction, not a classic rigid pushwall.

# 9. Automap / detector / pickup dispatcher

| Field/type | Meaning |
| --- | --- |
| `DS:41AF` | automap energy |
| `DS:4196` | automap enable; drains every 16 ticks; zero = auto-off |
| `DS:41AE` | enemy-detector energy |
| `DS:4197` | detector enable; drains every 8 ticks |
| Renderer mode 6 | live GUARDs on map |
| Renderer mode 7 | low-power static/noise |
| Reward 4 | refill automap to 100 |
| Reward 5 | refill detector to 100 |

Pickup/access dispatcher for types `0x2F–0x3D` is stable across DOS versions; in v2.0 it is around `~9E56` with jump table `CS:1A96`.

Observed effects include bitmask fields `4192/4193/4194/4195`, score-like `4182` rewards `+200/+250/+500`, bounded health/ammo, and events `04,05,06,08,0B–0F`.

# 10. Renderer and frame buffers

| Structure | Address / size | Note |
| --- | --- | --- |
| SPAN count | `DS:4D64` | max 50 |
| SPAN records | `DS:4D6E` | stride `0x12 = 18 B`; total 910 B |
| OWNER | `DS:4564` | 320 WORD |
| VEC count | `DS:626E` |  |
| VEC pool | `DS:62AC` |  |
| Shade/remap | `DS:6278` |  |
| Framebuffer | `A000:0000..F9FF` | 64,000 B at 320×200×8 |
| Palette | — | 768 B |

| Breakpoint / anchor | Meaning |
| --- | --- |
| `BF36` | frame begin |
| `1021E` | visibility |
| `213E / BF45` | owner/span entry / span complete |
| `BF5F` | walls complete |
| `C26F` | post-present |

Runtime capture: in an older build capture, the inner loop was found at `0977:05C6–05D5`: `mov AL,[SI] → XLAT → STOSB`, followed by `DI += 0x4F`; after `STOSB` the effective vertical step is `0x50`. `ES=A000`. This segment/address is build-specific.

# 11. Scheduler/timing anchors

| Anchor | Role |
| --- | --- |
| `BCC2` | IRQ8 |
| `BE74` | clock |
| `BEB4` | slow bucket |
| `BEF4` | frame bucket |
| `C150` | FAST tick |
| `C1A8` | outer scheduler |
| `C0D8` | MAIN/update/render path |
| `DS:081E/0820` | 1024 Hz origin clock |

High-confidence static ordering hypothesis: FAST includes auto-close/guards/timers/player/object/hazard/HUD; MAIN includes render, powerups, secret wall, projectiles, present and input.

For 1:1 parity, exact runtime interleaving and the first-divergence tick still need to be verified, especially for pause/resume, `>500` watchdog resync and 32-bit wrap.
