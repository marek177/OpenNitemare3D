# Original runtime delta — 2026-09-22

Reference: `marek177/Nitemare3d-reversed/docs/REMOTE_DOORS_AND_DOS_CROSSCHECK_2026-09-22.md` and `marek177/nite3d-dos/docs/DOS_RUNTIME_FINDINGS_2026-09-22.md`. The original Win16 logic is a target for this C# remake; this note does not imply these features are wired into Game.cs.

## Implement in this order

1. Resolve the object class-3 control from wall class `0x03`; check ID-card mask bit by its group index.
2. Expose menu actions `0x1E/0x1F` for matching remote door classes `0x3B/0x3C`; keep each door's state separate from the toggled group UI bit.
3. Handle wall-property `0x40` by episode/level script dispatch, independent of remote door UI.
4. Add explicit WARP class ranges `0x0D–0x2C`, including four color keys and one-way S1→S2 with four pentagrams.
5. Reproduce exploding-wall class chain `0x2E/0x2F → 0x2D` and SFX `0x29` after verifying the final map/collision writes in raw DOS assembly.
6. Add regression scenarios from captured original-game traces for door open/close, card gating, S1/S2, and explosion completion.

`RecoveredInteractionFacts.cs` contains data-only class/command constants and pure guards for the confirmed part of this path. It does not mutate current game state. DOS and Win16 behavior may differ; raw-assembly checks remain necessary before claiming exact timing and final collision behavior.

Current `RecoveredMechanics.cs` declares an offset `0x18` in a 28-byte object record; treat that name/meaning as provisional until its reader and writer are independently traced. Also review health addition at 99 HP: the Win16 USE routine visibly tests `<100` before +20, without saturating inside that routine.
