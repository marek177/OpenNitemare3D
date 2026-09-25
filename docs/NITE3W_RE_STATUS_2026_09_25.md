# Nite3W / Nitemare 3D reverse-engineering status — 2026-09-25 late update

This is a working coverage snapshot, not a claim that the original source code has been recovered.

## Evidence policy

- **CONFIRMED** — directly supported by disassembly/data/runtime evidence.
- **STRONGLY INFERRED** — multiple independent clues agree, but one decisive runtime/static link is still missing.
- **PARTIAL** — important portions are mapped, but behavior is not complete enough for 1:1 parity.
- **OPEN** — insufficient evidence for a faithful reconstruction.

## Current subsystem coverage

| Subsystem | Coverage | State | Main remaining work |
|---|---:|---|---|
| Renderer — static analysis | 80–85% | PARTIAL | Pixel-perfect projection/clipping/draw ordering, dynamic parity tests |
| Player movement / collision | 80–90% | PARTIAL | Corner/slide edge cases, moving-door/object collision parity |
| USE / interactions | 75–90% | PARTIAL | Full side effects for doors, switches, teleports, push/special walls |
| GUARD runtime / AI | 75–90% | PARTIAL | Remaining state-specific transitions, timing and target-selection edge cases |
| OBJECT runtime | 70–85% | PARTIAL | Complete per-class update behavior and ownership links |
| HUD / UIF / menus | 60–80% | PARTIAL | Exact UIF indices, redraw strategy, portrait thresholds, right panel |
| IMG / SEQDEF / animation | 65–80% | PARTIAL | Semantic mapping of every sequence/token |
| Sound → event/enemy mapping | 65–85% | PARTIAL | Exact trigger mapping for remaining effects/enemies |
| MFC / C++ runtime / init / memory | 45–65% | PARTIAL | CDC/GDI/Menu ownership, temp/permanent wrappers, destruction order |
| BSF / registration / integrity | 35–50% | OPEN | Registration checks, integrity semantics and version-dependent paths |

## Confirmed anchors relevant to integration

- Win16 USE-like input bit: **0x0200**.
- Player coordinate commit locations: **0x4BF6 / 0x4BF8**.
- Player HP source: **0x4C1D**.
- Known ammo/weapon-related locations include **0x4C1F / 0x4C20 / 0x4C44** and weapon selector **0x4C23**.
- Event flag block begins at **0x51A4**.
- Visible-span count/data are associated with **0x5E7E / 0x5E88**.
- Confirmed runtime record sizes currently used by the isolated RE module: OBJECT **28 B**, GUARD **26 B**, projectile slot **42 B**.
- The isolated RE module intentionally remains outside the production gameplay path until unresolved branches are closed.

## Renderer status

The renderer is no longer a low-confidence subsystem. The static data flow, visible-span structures and major raycasting/projection relationships are substantially mapped. The remaining gap is mainly **behavioral parity**, not basic architecture.

Do **not** mark renderer as 100% until at least these are closed:

1. exact clipping at viewport boundaries;
2. wall/sprite overlap ordering in all edge cases;
3. texture-column selection parity;
4. vertical projection rounding parity;
5. dynamic tests against captured Nite3W frames.

## HUD / menu status

Known menu/HUD code paths are sufficient to reconstruct a functional approximation, but not yet a verified 1:1 replacement. Outstanding uncertainties include:

- exact UIF.DAT item → HUD element index mapping;
- full-panel versus dirty-region redraw policy;
- exact portrait-health thresholds;
- purpose and population rules of the right black panel;
- remaining menu-state transitions and framework ownership.

## Debug mode / debug menu

Current evidence supports internal diagnostic/development-oriented paths, but a user-facing, fully reconstructed **debug menu** should not yet be declared confirmed without a complete call-chain and activation-condition proof.

## Integration rule

Production code should only consume behavior marked CONFIRMED. STRONGLY INFERRED/PARTIAL behavior belongs in isolated RE helpers, tests, or documentation until parity is demonstrated.

## Next closure targets

1. BSF / registration / integrity.
2. MFC ownership and object lifecycle.
3. HUD/UIF exact resource mapping.
4. USE side-effect matrix.
5. Renderer pixel-parity tests.
6. GUARD/OBJECT remaining state transitions.
