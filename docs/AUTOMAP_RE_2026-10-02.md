# Automap RE implementation sync — 2026-10-02

Branch: `automap-re-2026-10-02`

## Reverse-engineering status

The Nitemare-3D automap algorithm is now considered **CLOSED** at the algorithm level for the available DOS and Win16 builds.

### DOS request models

DOS v1.0 / v1.7 / v1.9 use the 9-request model:

| Request | Meaning |
|---:|---|
| 0 | draw wall / dynamic-door map geometry |
| 1 | erase dynamic-door geometry |
| 2 | clear persistent 64x64 automap |
| 3 | clear visible 62x36 panel |
| 4 | draw base map + blinking player |
| 5 | draw enemy markers |
| 6 | draw low-power Magic Eye noise |
| 7 | clear player cell |
| 8 | draw heading marker |

DOS v2.0 and Win16 use the 10-request model:

| Request | Meaning |
|---:|---|
| 0 | draw wall / dynamic-door map geometry |
| 1 | erase dynamic-door geometry |
| 2 | clear persistent 64x64 automap |
| 3 | clear visible 62x36 panel |
| 4 | damage flash |
| 5 | draw base map + blinking player |
| 6 | draw enemy markers |
| 7 | draw low-power Magic Eye noise |
| 8 | clear player cell |
| 9 | draw heading marker |

The later model is used as the canonical OpenNitemare3D implementation.

## Confirmed behavior represented in code

- persistent map is 64x64 bytes, indexed as `x * 64 + y`;
- visible HUD map is 62x36 at logical position 256,162;
- Magic Eye is toggled with F9;
- Crystal Ball is toggled with F10;
- each pickup adds 20 charge and is rejected at 100;
- Magic Eye consumes 1 charge every 16 nominal 8 Hz slow ticks;
- Crystal Ball consumes 1 charge every 8 slow ticks;
- Crystal Ball enemy markers blink by charge parity at values <= 15;
- Magic Eye noise count is `500 / power^3`;
- player marker blinks white and the previous player cell is cleared;
- heading marker is logical yellow;
- normal walls use logical green;
- dynamic doors use logical light blue;
- enemies and damage flash use logical light red;
- noise/player marker use logical white;
- Crystal Ball HUD gauge is dark red when inactive and light red when active;
- Magic Eye HUD gauge is `ResolveColor(2)-5` when inactive and logical light green when active;
- gauges are 18x7 pixels;
- Win16 logical-color resolver searches palette entries 10..245;
- DOS resolver searches entries 0..255.

## Files changed

- `Automap.cs` — automap state, palette resolver, power system, compositor and persistence hooks.
- `Input.cs` — F9/F10 keys.
- `Pickup.cs` — Magic Eye / Crystal Ball charge acceptance.
- `Player.cs` — wall-discovery feed from the current raycaster.
- `Program.cs` — automap update before HUD rendering and automap composition after the HUD PCX.
- `Game.cs` — automap state reset on game load.

## Current implementation boundaries

Two parts are intentionally not described as pixel-perfect yet:

1. **Wall discovery backend** — the original game feeds automap discovery from VEC geometry. Current OpenNitemare3D is still a tile/DDA renderer, so the branch records the visible hit tile using the recovered original logical wall/door colors.
2. **Noise RNG stream** — the original game uses its common runtime RNG. OpenNitemare3D currently has no recovered shared original RNG service, so Automap uses a deterministic local RNG until the game-wide RNG is reconstructed.

Dynamic-door draw/erase hooks (`DoorOpened`, `DoorClosed`) are present, but the current OpenNitemare3D sliding-door runtime is not yet a 1:1 implementation.

## Validation checklist

- build on the normal OpenNitemare3D .NET 5 environment;
- collect Magic Eye and Crystal Ball;
- verify full-charge pickup rejection;
- verify F9/F10 edge-triggered toggles;
- verify gauge color changes ON/OFF;
- verify Magic Eye depletion cadence;
- verify Crystal Ball depletion cadence;
- verify low-charge Crystal Ball marker parity;
- verify low-charge Magic Eye noise counts;
- compare logical RGB against original GAME.PAL;
- later replace DDA discovery with recovered VEC discovery;
- later replace local noise RNG with the shared original RNG stream.


## VEC discovery closure update

The branch now maps the existing DDA ray hit onto the original VEC orientation convention:

- 0 = top edge
- 1 = bottom edge
- 2 = right edge
- 3 = left edge

For ordinary wall classes 0x01..0x30, the automap records the corresponding tile-boundary edge and merges contiguous cells with the same raw wall ID/material, matching the original VEC run concept.

The MAP header is now read directly at offsets 0x0002..0x0101 to obtain the episode-specific 256-byte wall-ID -> class-ID table. This removes name-based door guessing.

Door classes 0x31..0x40 use the recovered special VEC rule:
- odd classes 0x31,0x33,...,0x3F accept vertical orientations 2/3;
- even classes 0x32,0x34,...,0x40 accept horizontal orientations 0/1;
- door automap color is logical 9;
- request-1 erase behavior is exposed through DoorOpened(x,y,orientation,flag20), including the VEC+5 bit-0x20 neighbour-cell rule.

Normal-wall boundary detection follows the original property split: a regular wall face borders an automap edge when the neighbour is not class 0x01..0x30. A neighbouring door therefore counts as an exposed normal-wall boundary, matching FUN_1018_4046.

## Original RNG

The local System.Random fallback has been removed.

OpenNitemare3D now has OriginalRandom.cs implementing the exact Microsoft C RNG used by the reference executable:

    state = state * 0x343FD + 0x269EC3
    result = (state >> 16) & 0x7FFF

Seed 1 produces the known initial sequence:

    41, 18467, 6334, 26500, 19169

Automap low-power noise now consumes this shared stream. The new-game load path resets it to seed 1, matching the recovered Nitemare-3D initialization path.

## Remaining runtime gaps after this pass

- The main renderer is still DDA rather than the original full VEC/column-owner renderer. Automap discovery now uses original VEC edge semantics, but visibility ownership is still driven by the current DDA hit.
- The existing OpenNitemare3D dynamic-door runtime is not yet the recovered paired-VEC 1:1 implementation, so the exact DoorOpened(...flag20) hook is currently future-facing.
- Player damage is not yet implemented in the current gameplay code; therefore the recovered damage-flash hook cannot be triggered correctly without first wiring the real damage path.
- Runtime build/framebuffer validation is still required before merge.
