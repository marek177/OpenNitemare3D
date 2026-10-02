# C++ research translated to C#

Source: marek177/Nitemare3d-reversed at d2be7fb09d6b1b6e0404517b603b306c8f1096db.

| Research source | C# destination | Integration |
| --- | --- | --- |
| src/game/InventoryRuntime.hpp | RecoveredInventory.cs | Player stores key/card masks; Pickup grants bits before removal |
| src/formats/MapArchive.cpp | RecoveredMapArchive.cs | Level.LoadMap validates header/payload and selects a zero-based level before spawning |
| src/formats/DemoFile.cpp | RecoveredDemoFile.cs | Executable parser and one-event dispatcher; playback is not yet connected to player input |
| src/game/PlayerHealthRuntime.hpp | RecoveredPlayerHealth.cs | Executable receiver/value model; game death animation integration remains open |

The health helper preserves transient pickup values over 100 until HUD clamping,
unsigned lethal comparison, and the exact game-state-2 suppression. Potion color
bindings remain unknown in the source report, so no amounts are invented for them.
DEMO parsing is the recovered Win16 format; this does not assert DOS compatibility.

Removed FuseBox.cs: no references or construction sites exist, and its shoot/use/
update bodies were empty. It did not implement a fuse-box mechanic. Warp.cs is
retained because Level constructs it; Curtain.cs is retained because Level uses its
sound action. Base virtual hooks in Entity and Tile are intentional extension points.
The C++ reference directory is retained as evidence, not added as a build dependency.

Also fixed the y=64 map-boundary access in Level.IsWalkable.

Validation: run `dotnet run --project tests/recovered/RecoveredTests.csproj -c Release`.
The test project compiles the actual independent C# port files without SFML or NuGet
packages. It covers inventory gates, health transitions, malformed maps, selection
of the second level, DEMO timing ties, EOF and nonmonotonic generations. The game
project excludes tests from its default recursive compilation. GitHub Actions runs
this test project. Full SFML gameplay/death/playback validation remains separate.
