# Nitemare 3D RE — Open Gaps Toward 1:1 Parity

What remains unresolved after the discoveries of the last week.

**Period:** October 1–7, 2026

## Confidence legend

**CONFIRMED** — directly supported by disassembly/dumps or agreement between multiple artifacts.  
**STRONG** — very strongly supported statically, but a targeted runtime proof is still missing.  
**CORRECTION** — a new finding invalidates or refines an older hypothesis.  
**OPEN** — behavior/order is not yet closed by a 1:1 runtime test.

# 1. What is no longer the main problem

Finding approximately “where player movement / projectile pool / GUARD loop / secret wall lives” is no longer the main problem. Those areas now have concrete addresses and record models.

SND/UIF/DAT formats and a large part of DOS v2.0 static code are no longer the least-understood areas.

The lowest parity has shifted to exact timing, collision edge ordering, RNG chronology, boss/class-specific behavior, and pixel-perfect rendering.

# 2. Critical open gaps

| Priority | Area | Exactly what is missing | Best closure proof |
| --- | --- | --- | --- |
| P0 | Renderer pixel parity | fixed-point rounding, clipping edges, sprite/wall draw order | `A000` frame diff + first divergent SPAN/write |
| P0 | RNG/DEMO determinism | complete ordered calls from `seed=1`, including audio/visual branches | RNG breakpoint log with caller + state before/after |
| P0 | Scheduler/timing | exact FAST/MAIN interleaving and bucket boundaries | `C150/C1A8` trace for a single tick |
| P1 | GUARD AI | LOS edges, blocked response, state transitions, class/boss overrides | state-scoped trace + memory snapshot |
| P1 | Player collision/sliding | axis order and trigger/block combinations at boundaries | `6378/6488` watch + controlled map fixtures |
| P1 | Projectile edge cases | dynamic wall/door ordering, stale `+18/+19` cache, pool-full behavior | slot trace + before/after records |
| P1 | Secret panel | collision/passability during retraction, exact completion tick | `34F6 + VEC + MAP` tick trace |
| P2 | Boss-specific logic | immunity, transformations, attack/death sequences | class-isolated scenarios |
| P2 | Cross-version mapping | pair DOS v1.x/v2.0 and Win16 behavior without blind offsets | fingerprint + function signatures + behavior tests |

# 3. Unknown map — concrete points

Atypical DOS v2.0 function boundaries/secondary entries: for example `0800:0768` remains a candidate fall-through/secondary entry; several other frameless/complex blocks still need a boundary audit.

OBJECT runtime `0E50`: exact iteration order, timer decrement, movement/collision, and mutation/deactivation ordering for every class still need closure.

GUARD state machine: the main dispatcher skeleton is known, but not every class-specific state/nextstate override branch and boss transition has runtime proof.

Projectile damage coupling to projected-Y cache (`+18/+19` in the embedded OBJECT) is statically strong but behaviorally risky; it must be verified whether this is intentional mechanics or a stale-cache artifact.

Renderer: buffer architecture is already well understood, but pixel-perfect parity depends on exact signed/unsigned fixed-point rounding, overflow behavior and clipping order.

DEMO cross-version: the event-encoding/cadence difference is localized, but a complete DOS↔Win16 translation map is not yet closed.

`NITE3D.BSF`: decoding/lineage is substantially clearer, but the meaning of every internal record and all variant differences has not yet been fully named semantically.

# 4. Corrected mistakes that must not return to documentation/code

| Forbidden old assumption | Correct current state |
| --- | --- |
| Use `3:4BF6/4BF8` as universal player X/Y | DOS v2.0: `DS:4162/4164`; map other builds separately. |
| Use `7E5E`, `6EC8`, `9806` as universal offsets | They are build-specific. Fingerprint first. |
| Interpret a `+0x50` loop stride as OBJECT record stride | The concrete v20 sites are VGA/planar scanline stride. |
| Label `70D6` as the OBJECT scheduler | It is currently more likely an input/control/event dispatcher. |
| Implement secret panel as a rigid pushwall | Use VEC-segment retraction based on the 14-byte record and `0AEA`. |
| Count RNG only through a wrapper | Include direct far calls to `0FBA:01A0` and rejection loops. |

# 5. Recommended closure order

| Order | Task | Why now |
| ---: | --- | --- |
| 1 | RNG ordering from DEMO `seed=1` to first input/tick | affects AI, animation, audio, damage and DEMO determinism simultaneously |
| 2 | Scheduler exact order `C150/C1A8` | without correct order every later subsystem can appear wrong |
| 3 | Player collision/sliding edge matrix | foundation of movement parity and DEMO replay |
| 4 | Secret-panel occupancy + collision during movement | record and handler are known; only runtime closure remains |
| 5 | Projectile pool + stale-cache edge cases | most of the pipeline is known; suitable for rapid closure |
| 6 | GUARD class/boss branches | higher complexity, but the base scheduler/state model exists |
| 7 | Renderer pixel diff | final 1:1 rendering closure after timing/RNG are stabilized |

# 6. Definition of Done for “100%”

**Static:** every Nitemare-owned basic block and relevant data record is named, assigned to the correct build, and has documented inputs/outputs/side effects.

**Behavioral:** every gameplay state transition has at least one reproducible runtime proof and an edge-case test.

**Timing:** the same input stream produces the same subsystem order and the same deadline/timer values after every tick.

**RNG/DEMO:** the same seed and input reproduce the same ordered RNG stream and replay without drift.

**Rendering:** the same state produces a byte-identical framebuffer and palette; `mismatch_count=0`.

**Original data:** `MAP/OBJECTS/WALLS/IMG/SND/UIF/BSF/SAV` are loaded without heuristic conversion, using the correct version-specific semantics.

# 7. Conclusion

After this week’s findings, the main value is that the remaining unknowns are small, named and experimentally measurable. The path to 1:1 is no longer “reverse-engineer the entire EXE again”; it is to systematically close first-divergence points subsystem by subsystem.
