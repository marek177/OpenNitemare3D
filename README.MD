# OpenNitemare3D
reimplementation of the 1994 FPS Nitemare 3D in C#
[Twitter](https://twitter.com/CatboiN).
[Discord](https://discord.gg/pr36YJ9Njw).
[Tumblr](https://www.tumblr.com/blog/bbqgiraffe).

# this is currently being rewritten in C for performance reasons, active development branch is C-Rewrite

[![Showcase](https://yt-embed.herokuapp.com/embed?v=KLNquQF4iJo)](https://www.youtube.com/watch?v=KLNquQF4iJo-Y "fucking uhhh")

# how to compile:
open the folder in vscode and press 'F5', you'll need the DotNet SDK
this will compile and run fine on anything with DotNet and SFML

# how to install:
you will need to get the original game files, I reccomend just getting them from archive.org since the [version on David Gray's website](https://www.dgray.com/n3dpage.htm) is newer and has unsupported file types, however you will need to get SND.DAT from the newer version as the old version uses unsupported .VOC sounds [archive to newer version(you only need SND.DAT)](https://archive.org/download/win3_Nite3d3x/win3_Nite3d3x.zip), place SND.DAT(windows version!!!), UIF.DAT MAP.1, IMG.1, and GAME.PAL in bin/Debug/net5.0/data, then you can just launch the executable, it's untested on Windows but it should probably work since it's DotNet, you will probably need to download SFML binaries

note that most of the menu options will crash the game, only new game and sound editor will do anything

# how to help:
if you for some reason want to contribute to my garbage you can join the discord,
I'm open to any help especially with reverse engineering the executable as I'm garbage at assembly,
I also need help labeling all the sound effects

if you're David P Gray please consider open-sourcing Nitemare 3D

## Reverse-engineering synchronization

This fork is being synchronized with verified clean-room findings from
[`marek177/Nitemare3d-reversed`](https://github.com/marek177/Nitemare3d-reversed).
Only findings backed by original NITE3W.EXE/data/save evidence are promoted to
runtime behavior; partially identified values remain documented rather than guessed.

The current branch synchronization is documented in [`REVERSE_ENGINEERING_SYNC.md`](REVERSE_ENGINEERING_SYNC.md). The source-of-truth repository now has a consolidated cross-thread/session ledger at `docs/ALL_THREADS_CONSOLIDATION_2026-09-22.md` plus dedicated renderer, GUARD, combat, collision, save and audio reports.

The synchronization now preserves much more than the original first pass:

* 64x64 MAP geometry, 514-byte header and 8192-byte level payloads
* 27-world-unit player collision half-extent and axis-separated sub-tile movement
* rising-edge USE/ACTION targeting one adjacent cardinal cell
* original OBJECT=28 B and GUARD=26 B record sizes
* fresh GUARD strength 255 and 22-state dispatcher range
* original GUARD score-switch values, including Dracula=0, Demon=250, Penelope=-1000 and Dr. Hamerstein=1000
* original difficulty value ordering and key player/weapon globals
* doors/panels/pushes/vector/span/sprite runtime capacities and strides
* the original renderer's 320x200 framebuffer and 304x152 game viewport
* VEC/VECLIST/owner/span/sprite-queue structural constants recovered from NITE3W.EXE

The original renderer is not a Wolfenstein 3-D one-ray-per-screen-column DDA. The recovered Windows path converts MAP boundaries into 28-byte VEC records, indexes them through four 333-entry orientation lists, projects/near-clips wall segments, resolves a 320-column owner table, coalesces visible columns into wall spans and renders projected sprites through a separate 100-entry queue. `RecoveredRuntime.cs` records the verified constants without pretending that the current C# renderer already implements that complete original pipeline.

## features:
* Loading of original bitmap images
* Loading of original maps
* Playback of MIDI music
* Playback of SFX from Windows version (some Dos SFX *will* play but most of them sound garbage)
* semi-functional main menu

## currently being worked on:
* full support for Dos sound files
* sprites
* implementing curtains, warps, hidden panels
* tile editor that auto-generates code for me
* continued runtime reconstruction from Nitemare3d-reversed

### file formats:

#### DAT:
Nitemare 3D uses the .DAT format(also used by David Gray in the Hugo series),
this consists of a header that contains offsets for every file stored in the container, the offset consists of a unsigned 16 bit integer for the length followed by a unsiged 32 bit integer for the position.
<br></br>

For the audited Windows SND.DAT, the fixed directory is 160 records x 6 bytes. IDs 1..15 are MIDI. The logical SFX range is 34..110 and the original Windows playback format recovered from NITE3W.EXE is raw mono 8-bit PCM at 11025 Hz. This matters because treating the Windows samples as generic VOC data can produce incorrect playback.

UIF.DAT consists of indexed UI resources; its complete per-ID semantics are still under reverse-engineering and should not be guessed from the current reimplementation alone.

#### MAP:
MAP.1, MAP.2 and MAP.3 are binary archives. The archive header is 514 bytes;
after it, every level occupies exactly 8192 bytes (`64 * 64 * 2`). Each map cell
is an interleaved two-byte pair `{wallByte, objectByte}`. MAP.1 contains 11 level
payloads (E1M11 is the internal/demo map); MAP.2 and MAP.3 contain 10 each.
Runtime world coordinates use 64 world units per map tile.

The 11/10/10 counts describe the supplied original archives; they are not treated as a proven universal file-format ceiling. Historical MapEdit code supports a larger editor level count, while the exact original runtime archive ceiling remains a separate reverse-engineering question.
