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
