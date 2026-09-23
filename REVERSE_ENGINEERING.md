# Reverse-engineering integration

This fork is being updated from findings independently reconstructed from the original
Windows 3.1 NITE3W.EXE and original game data. The working evidence is maintained in
marek177/Nitemare3d-reversed.

## Implemented constants / classifications

`ReverseEngineeredRules.cs` records verified executable-level findings so the C# port
does not continue to rely on guessed values:

- USE/ACTION input mask is `0x0200` and is rising-edge triggered in the original.
- USE targets one adjacent cardinal MAP cell after octant quantization.
- World scale is 64 internal units per tile; original player collision half-extent is 27.
- Runtime limits/strides: doors 64 x 22 bytes, panels 32 x 22 bytes, pushables 12 x 6 bytes.
- Dynamic-door wall property bit is `0x08`; generic blocking property bit is `0x02`.
- Logical wall type `0x03`: ID-card remote doors/cannons terminal.
- `0x0D..0x14`: Climb up / Climb down / Cancel family.
- `0x15..0x18`: Other Side / mirror / pentagram family.
- `0x19..0x1C`: reusable Red/Green/Blue/Yellow key gates.
- `0x1D..0x24`: Floor 1..Floor 10 selector family.
- `0x25..0x2C`: Go down / Cancel family.
- Combination input is logical object type `0x26`, not wall `0x25..0x2C`.
- Pushable object type is `0x28`; push movement is eight 8-unit updates per tile.
- Colored key order is Red, Green, Blue, Yellow. ID-card order is Red, Yellow.

## Important fidelity rule

Unknown values are deliberately not invented. In particular, exact enemy movement,
remaining AI transitions, enemy-to-player damage, some weapon timing, and unresolved
OBJECT semantics should remain marked as unknown until executable evidence settles them.

The current OpenNitemare3D gameplay code contains several provisional values (for
example guard health/range/attack timing). Those should be replaced incrementally as
the corresponding original algorithms are verified.
