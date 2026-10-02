# Upstream legacy integration audit — 2026-10-02

## Scope

This fork originates from `BBQGiraffe/OpenNitemare3D`.

Relevant upstream history:
- original repository began in 2019;
- the current C# architecture was largely developed during 2021;
- upstream branches include `master`, `development`, and `C-rewrite`;
- `C-rewrite` continued through 2022 and is a separate unfinished SDL/C engine skeleton.

This audit records where executable-backed Nitemare 3-D research can replace
historical approximations without treating the old implementation as evidence
for original behavior.

## Highest-value discovery: C-rewrite

The upstream `C-rewrite` branch is structurally useful because its subsystem
split closely matches the recovered original engine:

| Upstream file | Historical state | RE-backed replacement/integration target |
|---|---|---|
| `g_think.c` | only dispatches player think | GUARD scheduler, 22-state dispatcher, projectile/object updates |
| `m_obj.c/.h` | monolithic float `obj_t`, MAXOBJ=256 | 350 x 28 B OBJECT pool + 100 x 26 B GUARD pool, world coordinates in 64-unit tile scale |
| `p_think.c` | incomplete movement, no recovered collision | player input masks, 27-unit AABB, 1-world-unit stepping, X/Y slide |
| `g_game.c` | partial MAP loading / spawn logging | exact 514-byte MAP header, 8192-byte levels, correct per-level offset and MAP-object -> runtime-class mapping |
| `r_raycaster.c` | placeholder test output | recovered MAP -> VEC -> VECLIST -> wall-owner -> span -> sprite pipeline |
| `r_img.c` | heuristic IMG offset scanning / rotation | verified IMG directory/layout and x-major pixel storage |
| `r_ui.c` | partial UI | exact UIF/HUD/automap dispatcher behavior and resource indices |
| `d_dat.c` | generic DAT reader | verified SND/UIF directory handling and validation |
| `i_sound.c` | MIDI-centric implementation | original MIDI/SFX event mapping and exact SND/VOC behavior |
| `g_inventory.h` | interface only | keys/cards, weapon ownership, ammo pools/caps and pickup semantics |
| `m_weapondata.h` | fire callback only | selectors 0..3, shared laser ammo, wand/silver ammo, cadence, hitscan/projectile split |
| `m_maptype.h` | strong map-object ID catalogue | retain IDs; bind directly to recovered OBJECT classes/runtime flags |
| `r_sprite.c` | incomplete helpers | exact IMG sprite dimensions/layout/transparency and projection inputs |

## Important direct findings from old source

### C-rewrite / `m_obj.h`
Historical values are incompatible with the original runtime:
- `MAXOBJ 256` should not be treated as an original limit;
- recovered original capacity is 350 OBJECT records;
- GUARD is a separate 100-record pool;
- historical `float x,y` is a port abstraction while original world X/Y are
  signed runtime coordinates with 64 world units per tile.

Recommended approach: retain a modern wrapper only if useful, but make
recovered OBJECT/GUARD records the authoritative gameplay state.

### C-rewrite / `p_think.c`
The historical code currently writes both velocity axes from `sin(angle)` and
does not implement original collision. This function is a clean replacement
target for the recovered player movement chain.

### C-rewrite / `g_game.c`
The loader seeks to 514 bytes and calculates a per-level offset, but the
calculated offset is not applied before `fread`. Its map-object coordinate
calculation is also based on the byte index rather than the cell index.
This is a strong target for replacement with the verified MAP archive loader.

### C-rewrite / `r_raycaster.c`
This is only a placeholder drawing a small palette block. It can be replaced
without preserving a guessed Wolf-style renderer. The recovered original uses
the vector/span architecture:
`MAP -> exposed boundaries -> VEC[1000] -> 4x VECLIST[333] -> projection ->
320-column owner table -> <=50 wall spans -> <=100 projected sprites`.

### C-rewrite / `r_img.c`
The legacy implementation uses heuristic offset scanning and an
`imgCount-1127` workaround. Replace this with verified IMG facts:
- wall directory offset 0x0000;
- object directory offset 0x0400;
- 256 entries per directory;
- 10-byte frame header;
- first frame stream around 0xBC00 in the audited data;
- image pixels are x-major: `x * height + y`.

### C-rewrite / `r_ui.c`
One historical detail agrees with current executable research:
the UI sprite path treats palette index 41 / `0x29` as transparent.
This should be preserved unless a resource-specific path proves otherwise.

The rest can be expanded with recovered:
- 32 UIF directory slots, 6 bytes each;
- HUD object/image bank 0xFF;
- 29 HUD frames;
- portrait selection;
- ammo/key/card gauges;
- automap buffer and dispatch rules.

### C-rewrite / projectile and weapon path
The old C branch has almost no real weapon runtime yet, making it a clean target
for:
- 8 x 42-byte projectile pool;
- 14-byte movement prefix + embedded 28-byte OBJECT;
- states 0 free / 1 flying / 2 impact;
- Silver Pistol hitscan vs projectile weapons 0/1/3;
- guard projectile hit tolerance +/-9 world units;
- class x weapon resistance matrix;
- difficulty transform;
- first-free-slot-before-ammo-consumption behavior.

## C# master/development integration

The 2021 C# code remains useful for UI, resource loading and rapid testing, but
several systems are explicitly approximations:
- `Player.RenderRaycaster`: Wolf-style DDA, not original NITE3W renderer;
- `Guard.cs`: six-state gameplay abstraction vs original 22-state GUARD FSM;
- `DirectionalGuard.cs`: useful historical patrol prototype, not authoritative;
- `Projectile.cs`: float-speed Entity projectile, not the original 8-slot runtime;
- `HiddenPanel.cs`: useful UI/gameplay shell, but original panel/door records are separate fixed pools;
- `Level.cs`: hand-authored wall/object switch is useful as a resource catalogue but should consume recovered runtime metadata.

Current branch `re-sync-runtime-2026-10-02` is already migrating these C#
systems through an evidence-backed runtime bridge.

## Recommended architecture

Do not mix the C and C# implementations in one production branch.

Keep two targets:

1. **C# integration branch**
   - fastest path to a playable corrected OpenNitemare3D;
   - continue replacing guessed behavior behind recovered runtime APIs.

2. **C-rewrite revival branch**
   - best long-term target for a cleaner, low-level, fidelity-oriented engine;
   - import/recreate the upstream C-rewrite tree in a dedicated branch and
     implement the recovered structures directly.

The same regression facts/self-tests should be shared conceptually between both
implementations.

## Evidence policy

Historical OpenNitemare3D code is a comparison/porting target, not evidence for
the original game. Where it conflicts with executable/data/runtime evidence,
the recovered NITE3W behavior takes precedence.
