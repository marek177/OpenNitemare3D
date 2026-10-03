using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text.Json;

namespace Nitemare3D
{
    /// <summary>
    /// Renderer Stage 4:
    /// persistent original VEC runtime + paired/class-3 geometry mutation +
    /// exact wall IMG sequence/frame selection.
    ///
    /// This stage stops rebuilding static VECs every rendered frame. The same
    /// VEC objects are retained and mutated by the reconstructed wall runtime.
    /// </summary>
    public static class OriginalRendererStage4
    {
        // Keep Stage 4 explicit until every world-object sprite class is
        // migrated into the recovered 18-byte queue. This avoids silently
        // hiding legacy-only pickups/scenery merely because renderer tables
        // happen to be present in data/.
        public static bool Enabled { get; private set; } = false;

        public static bool ForceNativeResolution
        {
            get { return Enabled; }
        }

        public static int LastVectorCount { get; private set; }
        public static int LastOwnedColumnCount { get; private set; }
        public static int LastSpanCount { get; private set; }
        public static int LastPairedWallCount { get; private set; }
        public static int LastClass3GroupCount { get; private set; }

        static string trigPath =
            Path.Combine(
                "data",
                "N3D_TRIG_Q10.BIN");

        static string visibilityPath =
            Path.Combine(
                "data",
                "N3D_VISIBILITY_OCTANTS.BIN");

        static string statsPath;

        static OriginalTrigQ10 trig;
        static OriginalVisibilityOctants visibility;

        static OriginalMapTables mapTables;
        static OriginalImgWallRuntime wallImages;
        static OriginalWallRuntime wallRuntime;

        static List<OriginalRendererCore.Vec> vectors;
        static List<OriginalRendererCore.Vec>[] orientationLists;

        static int loadedEpisode = -1;
        static int loadedLevel = -1;

        static readonly OriginalRendererCore.WallSamplingTables sampling =
            OriginalRendererCore.MakeWallSamplingTables(
                OriginalRendererCore.ViewportHeight);

        static readonly OriginalRendererCore.Vec[] ownerBuffer =
            new OriginalRendererCore.Vec[
                OriginalRendererCore.ScreenWidth];

        static readonly ushort[] wallVisibilityQ4 =
            new ushort[
                OriginalRendererCore.ScreenWidth];

        static OriginalVisibilityExact.Result lastVisibility;

        // Original RNG is a separate engine subsystem. Stage 4 refuses to
        // silently substitute System.Random for extended wall SEQDEF choices.
        public static Func<byte> RandomByteProvider { get; set; }

        public static void Initialize(
            string[] args)
        {
            bool explicitlyRequested = false;

            if (args == null)
                args = Array.Empty<string>();

            for (int i = 0;
                i < args.Length;
                i++)
            {
                if (args[i] ==
                    "--legacy-renderer")
                {
                    Enabled = false;
                }
                else if (args[i] ==
                    "--n3d-renderer-stage4")
                {
                    Enabled = true;
                    explicitlyRequested = true;
                }
                else if (
                    args[i] ==
                        "--n3d-trig" &&
                    i + 1 < args.Length)
                {
                    trigPath =
                        args[++i];
                }
                else if (
                    args[i] ==
                        "--n3d-visibility-octants" &&
                    i + 1 < args.Length)
                {
                    visibilityPath =
                        args[++i];
                }
                else if (
                    args[i] ==
                        "--n3d-renderer-stats" &&
                    i + 1 < args.Length)
                {
                    statsPath =
                        Path.GetFullPath(
                            args[++i]);
                }
            }

            if (!Enabled)
            {
                return;
            }

            if (!OriginalRuntimeState.TryLoadExactTrigQ10(
                    trigPath))
            {
                // Stage 4 is explicit while non-runtime world-object sprites
                // are still being migrated. Never substitute approximate trig
                // data when the user requested the recovered renderer.
                if (explicitlyRequested)
                {
                    throw new FileNotFoundException(
                        "Exact Nitemare3D Q10 trig table is required.",
                        trigPath);
                }

                Enabled = false;
                return;
            }

            trig =
                OriginalRuntimeState.ExactTrigQ10;

            if (File.Exists(visibilityPath))
            {
                visibility =
                    OriginalVisibilityOctants.Load(
                        visibilityPath);
            }
            else
            {
                string[] exeCandidates =
                {
                    "data/NITE3W.EXE",
                    "NITE3W.EXE",
                    "data/NITE3W-10.EXE",
                    "NITE3W-10.EXE"
                };

                visibility = null;

                foreach (string exePath in exeCandidates)
                {
                    if (!File.Exists(exePath))
                        continue;

                    visibility =
                        OriginalVisibilityOctants.LoadFromNite3w110Exe(
                            exePath);
                    break;
                }

                if (visibility == null)
                {
                    if (explicitlyRequested)
                    {
                        throw new FileNotFoundException(
                            "Exact Nitemare3D visibility-octant table is required.",
                            visibilityPath);
                    }

                    Enabled = false;
                    return;
                }
            }

            Console.WriteLine(
                "Original N3D renderer Stage 4 enabled.");

            Console.WriteLine(
                "Persistent dynamic VECs + exact wall IMG sequence/frame selection enabled.");
        }

        public static bool TryRenderWalls(
            Player player)
        {
            if (!Enabled)
            {
                return false;
            }

            if (player == null ||
                GameWindow.frameBuffer == null)
            {
                return false;
            }

            if (GameWindow.width != 320 ||
                GameWindow.height != 200)
            {
                throw new InvalidOperationException(
                    "Stage 4 requires logical 320x200.");
            }

            EnsureRuntime();

            Array.Clear(
                wallVisibilityQ4,
                0,
                wallVisibilityQ4.Length);

            FillOriginalBackground();

            short playerX =
                OriginalRendererCore.Wrap16(
                    (int)Math.Round(
                        player.position.X * 64.0,
                        MidpointRounding.AwayFromZero));

            short playerY =
                OriginalRendererCore.Wrap16(
                    (int)Math.Round(
                        player.position.Y * 64.0,
                        MidpointRounding.AwayFromZero));

            int angle =
                QuantizePlayerAngle(
                    player.rotation);

            lastVisibility =
                OriginalVisibilityExact.BuildOwnerBuffer(
                    orientationLists,
                    ownerBuffer,
                    playerX,
                    playerY,
                    angle,
                    trig,
                    visibility);

            LastOwnedColumnCount =
                lastVisibility.OwnedColumns;

            List<OriginalRendererCore.Span> spans =
                BuildSpans();

            LastSpanCount =
                spans.Count;

            uint nowMs =
                OriginalRuntimeState.RuntimeClockMs;

            DrawWallSpans(
                spans,
                playerX,
                playerY,
                angle,
                nowMs);

            // Original scheduler renders the current geometry and advances
            // moving wall geometry afterwards for the next frame.
            wallRuntime.TickPairedWallMotion();
            wallRuntime.TickClass3Motion();

            WriteStats(
                angle,
                playerX,
                playerY);

            return true;
        }

        static void EnsureRuntime()
        {
            if (Level.originalMap == null ||
                Level.originalVectors == null ||
                Level.originalWalls == null ||
                Level.originalWallImages == null)
            {
                throw new InvalidOperationException(
                    "Stage 4 requires Level.LoadMap original runtime state.");
            }

            bool sameRuntime =
                Object.ReferenceEquals(
                    vectors,
                    Level.originalVectors) &&
                Object.ReferenceEquals(
                    mapTables,
                    Level.originalMap) &&
                loadedEpisode == Game.episode &&
                loadedLevel == Game.level;

            mapTables =
                Level.originalMap;

            vectors =
                Level.originalVectors;

            wallImages =
                Level.originalWallImages;

            wallRuntime =
                Level.originalWalls;

            if (!sameRuntime ||
                orientationLists == null)
            {
                orientationLists =
                    BuildOrientationLists(
                        vectors);
            }

            LastVectorCount =
                vectors.Count;

            LastPairedWallCount =
                wallRuntime.PairedWalls.Count;

            LastClass3GroupCount =
                wallRuntime.Class3Groups.Count;

            loadedEpisode =
                Game.episode;

            loadedLevel =
                Game.level;
        }

        static int TextureIndexAt(
            int x,
            int y)
        {
            // Retained only as bridge metadata/debugging. Stage 4 wall pixels
            // now come from OriginalImgWallRuntime, not Img.current.entries.
            Tile tile =
                Level.tilemap[x, y];

            return
                tile == null
                ? -1
                : tile.textureID;
        }

        static int QuantizePlayerAngle(
            float radians)
        {
            int degrees =
                (int)Math.Round(
                    radians *
                    180.0 /
                    Math.PI,
                    MidpointRounding.AwayFromZero);

            // Port rotation 0 = east. Original N3D angle 0 = north.
            return
                OriginalTrigQ10.Normalize(
                    degrees + 90);
        }

        static void FillOriginalBackground()
        {
            const byte ceiling = 0x11;
            const byte floor = 0x0C;

            for (int x =
                    OriginalRendererCore.ViewLeft;
                x <=
                    OriginalRendererCore.ViewRight;
                x++)
            {
                for (int y =
                        OriginalRendererCore.ViewTop;
                    y <
                        OriginalRendererCore.CenterY;
                    y++)
                {
                    GameWindow.frameBuffer[x, y] =
                        ceiling;
                }

                for (int y =
                        OriginalRendererCore.CenterY;
                    y <=
                        OriginalRendererCore.ViewBottom;
                    y++)
                {
                    GameWindow.frameBuffer[x, y] =
                        floor;
                }
            }
        }

        static List<OriginalRendererCore.Vec>[]
            BuildOrientationLists(
                List<OriginalRendererCore.Vec> source)
        {
            List<OriginalRendererCore.Vec>[] lists =
            {
                new List<OriginalRendererCore.Vec>(),
                new List<OriginalRendererCore.Vec>(),
                new List<OriginalRendererCore.Vec>(),
                new List<OriginalRendererCore.Vec>()
            };

            foreach (OriginalRendererCore.Vec vec
                in source)
            {
                lists[
                    vec.Orientation].Add(
                        vec);
            }

            for (int i = 0;
                i < 4;
                i++)
            {
                if (lists[i].Count >
                    OriginalRendererCore.MaxOrientationList)
                {
                    throw new InvalidOperationException(
                        "MAXVECLIST exceeded (333).");
                }

                lists[i].Sort(
                    OriginalRendererCore.CompareOrientationList);
            }

            return lists;
        }

        static List<OriginalRendererCore.Span>
            BuildSpans()
        {
            List<OriginalRendererCore.Span> spans =
                new List<OriginalRendererCore.Span>();

            int x =
                OriginalRendererCore.ViewLeft;

            while (x <=
                OriginalRendererCore.ViewRight)
            {
                OriginalRendererCore.Vec owner =
                    ownerBuffer[x];

                if (owner == null)
                {
                    x++;
                    continue;
                }

                int start = x;

                while (
                    x + 1 <=
                        OriginalRendererCore.ViewRight &&
                    Object.ReferenceEquals(
                        ownerBuffer[x + 1],
                        owner))
                {
                    x++;
                }

                int end = x;

                if (spans.Count >=
                    OriginalRendererCore.MaxSpans)
                {
                    throw new InvalidOperationException(
                        "MAX visible spans exceeded (50).");
                }

                OriginalRendererCore.Span span =
                    new OriginalRendererCore.Span();

                span.Owner = owner;
                span.XStart =
                    OriginalRendererCore.Wrap16(
                        start);

                span.XEnd =
                    OriginalRendererCore.Wrap16(
                        end);

                if (!OriginalRendererCore
                    .InitializeSpanInterpolation(
                        span,
                        owner.ScreenX1,
                        owner.ProjectedY1Q4,
                        owner.ScreenX2,
                        owner.ProjectedY2Q4,
                        OriginalRendererCore.CenterYQ4))
                {
                    throw new InvalidOperationException(
                        "Span interpolation overflow.");
                }

                spans.Add(
                    span);

                x++;
            }

            return spans;
        }

        static void DrawWallSpans(
            List<OriginalRendererCore.Span> spans,
            short playerX,
            short playerY,
            int angle,
            uint nowMs)
        {
            foreach (OriginalRendererCore.Span span
                in spans)
            {
                OriginalImgWallRuntime.Frame frame =
                    wallImages.SelectFrame(
                        span.Owner);

                uint accumulator =
                    span.YAccumulator16_16;

                for (int x = span.XStart;
                    x <= span.XEnd;
                    x++)
                {
                    ushort spanYQ4 =
                        (ushort)(
                            accumulator >> 16);

                    short displacementQ4 =
                        unchecked(
                            (short)spanYQ4);

                    int bottomQ4 =
                        OriginalRendererCore.CenterYQ4 +
                        displacementQ4;

                    int visibilityValue =
                        OriginalRendererCore.CenterYQ4 +
                        displacementQ4;

                    if (visibilityValue < 0)
                        visibilityValue = 0;
                    else if (visibilityValue > ushort.MaxValue)
                        visibilityValue = ushort.MaxValue;

                    if (x >= 0 &&
                        x < wallVisibilityQ4.Length)
                    {
                        wallVisibilityQ4[x] =
                            (ushort)visibilityValue;
                    }

                    int topQ4 =
                        OriginalRendererCore.CenterYQ4 -
                        displacementQ4;

                    int top =
                        topQ4 >> 4;

                    int bottom =
                        bottomQ4 >> 4;

                    int fullHeight =
                        bottom -
                        top +
                        1;

                    if (fullHeight >= 1)
                    {
                        if (fullHeight > 511)
                        {
                            fullHeight = 511;
                        }

                        int firstY =
                            Math.Max(
                                OriginalRendererCore.ViewTop,
                                top);

                        int lastY =
                            Math.Min(
                                OriginalRendererCore.ViewBottom,
                                bottom);

                        if (firstY <= lastY)
                        {
                            DrawOneWallColumn(
                                span.Owner,
                                frame,
                                x,
                                firstY,
                                lastY,
                                fullHeight,
                                spanYQ4,
                                playerX,
                                playerY,
                                angle);
                        }
                    }

                    accumulator =
                        unchecked(
                            accumulator +
                            (uint)span.YStep16_16);
                }

                // This is where original 66B0 invokes 65A6.
                wallImages.UpdateAfterVisibleSpan(
                    span.Owner,
                    nowMs,
                    RandomByteProvider,
                    vec =>
                        wallImages.CompleteExplodingWall(
                            vec,
                            mapTables,
                            vectors));
            }
        }

        static void DrawOneWallColumn(
            OriginalRendererCore.Vec vec,
            OriginalImgWallRuntime.Frame frame,
            int screenX,
            int firstY,
            int lastY,
            int fullHeight,
            ushort spanYQ4,
            short playerX,
            short playerY,
            int angle)
        {
            if (frame.Width == 0 ||
                frame.Height < 64)
            {
                throw new InvalidDataException(
                    "Wall frame must provide a 64-texel vertical column.");
            }

            ushort u =
                OriginalTextureUExact.ComputeTextureU(
                    vec,
                    OriginalRendererCore.Wrap16(
                        screenX),
                    spanYQ4,
                    playerX,
                    playerY,
                    angle,
                    trig,
                    frame.Width);

            int texX =
                u;

            if (texX >= frame.Width)
            {
                throw new InvalidDataException(
                    "6422 texture-U is outside frame width; non-power-of-two wall resource?");
            }

            byte[] column =
                new byte[64];

            for (int y = 0;
                y < 64;
                y++)
            {
                column[y] =
                    frame.Pixels[
                        texX,
                        y];
            }

            uint source =
                OriginalRendererCore
                    .ClippedStart16_16(
                        sampling,
                        fullHeight);

            uint step =
                sampling.Step16_16[
                    fullHeight];

            int pixelCount =
                lastY -
                firstY +
                1;

            byte[] remap =
                OriginalRuntimeState.ShadeRuntime.ShadeIndex == 0
                    ? null
                    : OriginalRuntimeState.ShadeRuntime.Remap;

            OriginalRendererCore.DrawIndexedColumn(
                GameWindow.frameBuffer,
                screenX,
                firstY,
                pixelCount,
                column,
                source,
                step,
                remap);
        }

        public static void CopyWallVisibilityQ4(
            ushort[] destination)
        {
            if (destination == null ||
                destination.Length <
                    wallVisibilityQ4.Length)
            {
                throw new ArgumentException(
                    "320-entry wall visibility destination is required.",
                    nameof(destination));
            }

            Array.Copy(
                wallVisibilityQ4,
                destination,
                wallVisibilityQ4.Length);
        }

        // ---- Public gameplay/debug bridge for future USE dispatcher work ----

        public static bool TogglePairedWall(
            int mapX,
            int mapY,
            int playerSector)
        {
            if (!Enabled)
            {
                return false;
            }

            EnsureRuntime();

            return
                wallRuntime.TogglePairedWall(
                    mapX,
                    mapY,
                    playerSector);
        }

        public static bool ForcePairedWallState(
            int mapX,
            int mapY,
            short state)
        {
            if (!Enabled)
            {
                return false;
            }

            EnsureRuntime();

            return
                wallRuntime.ForceState(
                    mapX,
                    mapY,
                    state);
        }

        public static bool ActivateClass3Wall(
            int mapX,
            int mapY)
        {
            if (!Enabled)
            {
                return false;
            }

            EnsureRuntime();

            return
                wallRuntime.ActivateClass3(
                    mapX,
                    mapY);
        }

        public static bool SetWallAnimationInterval(
            byte wallId,
            ushort intervalMs)
        {
            if (!Enabled)
            {
                return false;
            }

            EnsureRuntime();

            return
                wallImages.SetIntervalForWallId(
                    wallId,
                    intervalMs);
        }

        public static void TickAutoClose(
            int playerTileX,
            int playerTileY,
            Func<int, int, bool> runtimeObjectOccupied)
        {
            if (!Enabled)
            {
                return;
            }

            EnsureRuntime();

            wallRuntime.TickAutoClose(
                playerTileX,
                playerTileY,
                runtimeObjectOccupied);
        }

        static void WriteStats(
            int angle,
            short playerX,
            short playerY)
        {
            if (string.IsNullOrEmpty(
                statsPath))
            {
                return;
            }

            Dictionary<string, object> stats =
                new Dictionary<string, object>();

            stats["episode"] =
                Game.episode;

            stats["level_zero_based"] =
                Game.level;

            stats["angle_deg"] =
                angle;

            stats["player_world_x"] =
                playerX;

            stats["player_world_y"] =
                playerY;

            stats["vector_count"] =
                LastVectorCount;

            stats["paired_wall_count"] =
                LastPairedWallCount;

            stats["class3_group_count"] =
                LastClass3GroupCount;

            stats["wall_sequence_cache_count"] =
                wallImages.Caches.Count;

            stats["owned_columns"] =
                LastOwnedColumnCount;

            stats["span_count"] =
                LastSpanCount;

            stats["map_path"] =
                mapTables.SourcePath;

            stats["img_path"] =
                wallImages.SourcePath;

            stats["stage"] =
                "persistent dynamic VEC + exact IMG low-wall sequence cache/frame selection";

            string directory =
                Path.GetDirectoryName(
                    statsPath);

            if (!string.IsNullOrEmpty(
                directory))
            {
                Directory.CreateDirectory(
                    directory);
            }

            File.WriteAllText(
                statsPath,
                JsonSerializer.Serialize(
                    stats,
                    new JsonSerializerOptions
                    {
                        WriteIndented = true
                    }));
        }
    }
}