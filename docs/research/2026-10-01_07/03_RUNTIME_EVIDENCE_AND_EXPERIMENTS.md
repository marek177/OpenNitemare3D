# Nitemare 3D RE — Runtime Evidence and Experiments

What can already be proven and what must still be measured for 1:1 parity.

**Period:** October 1–7, 2026

## Confidence legend

**CONFIRMED** — directly supported by disassembly/dumps or agreement between multiple artifacts.  
**STRONG** — very strongly supported statically, but a targeted runtime proof is still missing.  
**CORRECTION** — a new finding invalidates or refines an older hypothesis.  
**OPEN** — behavior/order is not yet closed by a 1:1 runtime test.

# 1. Evidence model

Static code equivalence is not enough for 100% behavioral compatibility. Every critical subsystem must be closed by a controlled runtime experiment.

Primary acceptance criterion: two implementations/builds start from the same input state, after one tick they have the same relevant state, and for rendering tests they produce an identical 64,000-byte framebuffer.

When divergence occurs, the target is not a “similar result”; it is the **FIRST DIVERGENCE TICK** and the first different write/call/branch.

# 2. Minimal deterministic AutoRE workflow

Preferred direction: use a simple deterministic **AutoRE Lite** instead of a large autonomous workflow: `memdumpbin`, breakpoint/watchpoint, call log, short CPU trace, registers and `clearbp`.

| Step | Action | Output |
| --- | --- | --- |
| 1 | Set build fingerprint and canonical DS/CS | correct symbols for the exact EXE |
| 2 | `BPOINT/BPMEM` on the target write/call | exact CS:IP trigger |
| 3 | `MEMDUMPBIN` before the event | `before.bin` |
| 4 | Execute one controlled input/tick | only one experimental variable |
| 5 | `LOGC` / short `TRACE` | call/branch chronology |
| 6 | `MEMDUMPBIN` after the event | `after.bin` |
| 7 | Byte diff + register snapshot | first changed field / first divergence |

# 3. DOSBox-X/RPC capabilities already available

Debugger and CPU trace are available.

Address spaces: segmented, linear, physical.

Memory-change breakpoints work for segmented and linear address spaces.

Reported RPC capability limits: `memory.read` up to 65,536 B, max message 1,048,576 B, max trace events 10,000.

RPC Connect has already worked; the practical problems were mainly the correct mount/working-directory profile and the strict address format required by `memory.read`.

# 4. Player movement/collision — closure experiment

| Experiment | Watch/break location | Dump | GREEN condition |
| --- | --- | --- | --- |
| Forward | `4162/4164 + 68B5/68B9` | player/state before+after | exact X/Y delta and identical map-cell commit |
| Wall collision | `6378 + 6488` | `D00C/D10C` cells | same hard-block branch and zeroed step |
| Sliding | `6378/6488/688C` | 2–3 neighboring cells + player | same axis order and resulting axis |
| Trigger cell | `D00C bit 0x40` branch | cell flags + callback state | side effect without incorrect blocking |

Goal: close footprint `0x1B`, exact primary/secondary cell order, and every combination of `0x04/0x08/0x40` versus secondary `0x04/0x02`.

# 5. Secret panel — closure experiment

| Phase | What to log |
| --- | --- |
| Before USE | record `34F6+n*0x0E`, 4 VEC endpoints, MAP cell |
| USE class 3 | branch `0867`, state transition, SFX `0x27`, initial ±1 offsets |
| Every `0AEA` tick | all 4 VEC endpoints, state, collision flags |
| Completion | `VEC.flags bit0`, MAP cell bytes, `state=0` |
| Occupancy test | player/GUARD/projectile in the space while retracting |

GREEN only if movement is exactly 2 world units/update after the initialization step, all segments finish on the same corresponding tick, and cleanup changes the same bits/cells.

Visual VEC movement must be separated from gameplay collision passability: pixel movement and collision state do not necessarily switch at the same instant.

# 6. GUARD AI + RNG ordering — closure experiment

| Scenario | Breakpoints/calls | Evidence |
| --- | --- | --- |
| State 06, both blocked | `525E / 5092 + RNG` | exactly 1 draw; bit0 selects X/Y flip |
| Planner `55A8` | `5666,56A6,573F` | exact timer/direction draws and difficulty scaling |
| State 13 | `58A7` | timer 8..87 + state transition |
| Non-lethal damage | `85AD + pain path` | damage RNG → pain/reaction ordering |
| Lethal damage | death variant + SFX RNG | exact draw count before death transition |
| Alert/wake | class-specific alert + wake timer | shared RNG stream and another `%8` draw |

Acceptance: from DEMO `seed=1`, the same ordered caller sequence and the same final RNG state must be reproducible. Audio/animation RNG must not be ignored.

# 7. Projectile/combat — closure experiment

| Test | Observe | Open detail |
| --- | --- | --- |
| Spawn into free slot | `7EDE + slot 41B6+n*2A` | exact initialization of all fields |
| Pool full | all 8 `state!=0` | whether ammo truly remains unchanged for every weapon |
| Wall hit | `8142 + cell flags` | collision order versus dynamic door/wall |
| GUARD hit | ±9 tolerance + damage path | GUARD scan order and multi-hit edge cases |
| Impact animation | state `1→2→0` | deadline semantics and overdue frame advance |
| Stale `+18/+19` cache | slot reuse/render visibility | whether cache changes damage and when it refreshes |

# 8. Renderer — pixel-perfect experiment

| Anchor | Capture |
| --- | --- |
| `BF36` | frame-begin state |
| `1021E` | visibility-decision state |
| `213E/BF45` | OWNER + SPAN before/after construction |
| `BF5F` | walls complete |
| `C26F` | post-present framebuffer |

Dump SPAN count `DS:4D64`, SPAN records `DS:4D6E` (`50×18 B`), OWNER `DS:4564`, VEC `DS:626E/62AC`, shade `DS:6278`.

Dump framebuffer `A000:0000`, length `FA00h = 64,000 B`, plus the 768-byte palette.

Stable frame → exactly one tick/input → second dump → byte diff. **GREEN: `mismatch_count=0`.**

If the frame diverges, the first different SPAN/OWNER/VEC write must be traced back to the fixed-point projection/clipping/rounding branch.

`A400`: `A000` is the confirmed VGA framebuffer segment. `A400` remains unconfirmed as a relevant Nitemare 3D runtime address and must not be treated as fact without new evidence.

# 9. Scheduler/timing — closure experiment

| Anchor | Test |
| --- | --- |
| `BCC2` IRQ8 | log every tick IRQ + clock write |
| `BE74` | verify 1024 Hz clock derivation |
| `BEB4` | slow-bucket boundaries |
| `BEF4` | frame-bucket boundaries |
| `C150` | FAST subsystem order |
| `C1A8` | outer scheduling |
| pause/resume | no hidden extra update |
| `>500` watchdog | resync behavior |
| 32-bit wrap | deadline comparisons across wrap |
| DEMO | identical input/RNG/tick ordering |

# 10. Acceptance metrics

**STATE_EQUAL:** relevant memory records and globals are byte-identical after the tick.

**RNG_EQUAL:** same seed → same ordered call list → same final RNG state.

**FRAME_EQUAL:** 64,000-byte framebuffer `mismatch_count = 0`; palette identical.

**ORDER_EQUAL:** same subsystem call order inside the tick; no extra draw/collision/update.

**FIRST_DIVERGENCE:** on failure, store the first different tick, CS:IP, branch and memory write — not only the final screenshot.
