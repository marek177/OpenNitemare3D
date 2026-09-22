# OpenNitemare3D reverse-engineering sync — 2026-09-22

This file records current implementation-relevant findings from `marek177/Nitemare3d-reversed` so OpenNitemare3D can converge toward the original game's behaviour without treating unverified guesses as facts.

## Confidence labels

- **CONFIRMED** — directly supported by original binary/data analysis or reproducible runtime observation.
- **PARTIAL** — subsystem/function is supported but some constants/fields remain unresolved.
- **INFERRED** — behaviourally plausible, not proven instruction-for-instruction.
- **UNKNOWN** — intentionally unresolved.

## Implementation targets

### Player / collision

- Health behaves as a bounded 0..100 gameplay value. **CONFIRMED/PARTIAL**
- Collision reconstruction should cover player radius/flags, wall collision, OBJECT/GUARD collision and sliding. **PARTIAL**
- Current Windows reverse-engineering work observes player X/Y commit locations around `0x4BF6/0x4BF8`; these are evidence anchors only, not values to encode in the C# port. **PARTIAL**

### OBJECT runtime

- Do not use an old 28-byte assumption for runtime objects. **CONFIRMED**
- Current iteration evidence indicates an approximately `0x50` / 80-byte runtime stride. **PARTIAL**
- Movement-related fields are observed around `+0x10/+0x12`. **PARTIAL**
- `+0x14/+0x18` remain unresolved high-value fields. **UNKNOWN/PARTIAL**

### GUARD runtime / AI

- Current runtime analysis indicates an approximately 98-byte guard structure. **PARTIAL**
- Behaviour still needing parity work: type/state transitions, HP, movement, attack, pain/death, timers and sound mapping. **PARTIAL**
- Keep runtime guard state distinct from static map/entity records. **IMPLEMENTATION RULE**

### Dracula -> Bat

A strong code-level finding indicates a distinct Dracula transformation path:

- Dracula transforms into internal `GUARD13 / type 0x14`. **STRONG PARTIAL**
- Normal Bat is type `0x08`. **PARTIAL**
- Transformation sets/restores 255 HP for the transformed form. **PARTIAL**
- The transformed form should therefore remain a separate runtime state/class until disproved. **IMPLEMENTATION TARGET**

### Combat / difficulty

- Enemy HP is per-guard. **CONFIRMED/PARTIAL**
- Exact HP tables and shots-to-kill tables remain incomplete. **UNKNOWN/PARTIAL**
- Damage parity needs weapon, range, difficulty, guard type and special/immunity correlations. **PARTIAL**
- Dr. Hammerstein should be audited as a boss-specific case rather than extrapolated from ordinary enemies. **PARTIAL**

### USE / interaction

- `0x0200` is an important USE/interaction-related value in the current reverse reconstruction. **PARTIAL**
- Original USE behaviour spans doors, switches, warp/teleport walls, push/special interactions, keyed interactions and special walls. **PARTIAL**
- The port should avoid reducing USE to a generic door-open command. **IMPLEMENTATION TARGET**

### Warp / doors / special walls

Known wall-function/name families include:

- `WARP`
- `WARP_L1`
- `WARP_L2`
- episode-specific `WARP_1..n`
- elevator/level warp variants
- `JAMB`
- vertical/horizontal door variants
- `SPECIAL1`
- `ONE_SHOOT`
- `REVWALL`
- `CONTROL`
- `LEVEL_UP`

`WARP_L1/WARP_L2` show evidence of key-dependent behaviour. Exact state/script semantics remain under XREF/runtime investigation. **PARTIAL**

Animated walls may use two or more animation ticks/frames. **PARTIAL**

### Renderer

- Studied Windows build uses a 320-pixel-wide framebuffer-oriented path. **CONFIRMED for that build**
- Current renderer work indicates a runtime record stride of about 52 bytes. **PARTIAL**
- Relevant code evidence under study includes locations around `0x5E7E`, `3:62F8`, and vectors at `4:34E1`, `4:3919`, `4:4073`. These are reverse-engineering anchors, not source-port constants. **PARTIAL**

### Data formats

Episode resource families:

- `MAP.1..3`
- `IMG.1..3`
- `WALLS.1..3`
- `OBJECTS.1..3`

Compatibility work should preserve IDs, object/guard placement, door and warp semantics, episode indexing, spawn/orientation, special walls and original EXE limits where known. **IMPLEMENTATION TARGET**

`E1M11` is used by `DEMO.1`; current comparison material indicates `E1M11` and `E1M3` are effectively identical. **PARTIAL/strong evidence**

### DEMO

- `DEMO.1` is associated with `E1M11`. **PARTIAL/strong evidence**
- Target representation: compact input stream mapping values to forward/back, turn, fire and use. **PARTIAL**
- Playback termination on key input and MIDI/music state changes are relevant to parity. **PARTIAL**
- `DEMO.2`/`DEMO.3` map identity remains unresolved. **UNKNOWN**

### SND.DAT

Audio parity requires comparison against actual in-game playback. Current reverse targets:

- MIDI/music mapping,
- SFX extraction,
- incorrect/failing VOC entries,
- `guard type -> attack/pain/death SND.DAT IDs`.

A file being technically playable is not sufficient proof that its decoding matches the original game. **IMPLEMENTATION RULE**

### ENDING.FLI

- Analysed file contains 488 frames. **CONFIRMED for current file**
- Relevant chunk types: `COLOR_64`, `BRUN`, `LC`, `BLACK`, `COPY`. **CONFIRMED/PARTIAL**
- Palette state and delta-frame dependencies must be preserved for correct decoding/editing. **IMPLEMENTATION TARGET**

### USER.SAV

Still only partially reconstructed. OpenNitemare3D parity should eventually cover:

- player state,
- map/episode state,
- inventory/ammo,
- serialized OBJECT/GUARD state,
- timers/flags,
- exact field offsets/sizes where established.

## Recommended implementation order

1. Player collision/sliding parity.
2. Runtime OBJECT model.
3. GUARD movement/state machine.
4. Player damage/death.
5. Weapon cadence/damage.
6. Enemy HP, pain/death and SFX mapping.
7. USE chain and door/switch/warp/push interactions.
8. DEMO command/value mapping.
9. USER.SAV compatibility.
10. Renderer parity and edge cases.

## Source-of-truth policy

ZDoom/GZDoom recreations, Wolf3D-family code and visual gameplay comparisons are useful behavioural references, but should not be marked as confirmation. Confirmation should come from the original Nitemare 3D executable/data or reproducible runtime observation.
