using System;
using System.Collections.Generic;
using System.IO;

namespace Nitemare3D
{
    /// <summary>
    /// Exact wall-side IMG sequence/frame runtime needed by 4C8A/65A6/66B0.
    ///
    /// Checked file layout:
    ///   0000..03FF  256 wall frame-stream offsets
    ///   0400..07FF  256 object frame-stream offsets
    ///   0800..61FF  256 x 90-byte low/wall SEQDEF
    ///   6200..BBFF  256 x 90-byte high/object SEQDEF
    ///   BC00..EOF   frame streams
    ///
    /// Wall cache entries are deduplicated by wall-directory stream offset,
    /// matching FUN_1010_4C8A.
    /// </summary>
    public sealed class OriginalImgWallRuntime
    {
        public sealed class Frame
        {
            public uint FileOffset;
            public byte Width;
            public byte Height;
            public byte[] Header =
                new byte[10];

            public byte[,] Pixels;
        }

        public sealed class SequenceCache
        {
            public byte SourceWallId;
            public uint StreamOffset;
            public ushort IntervalMs;
            public byte FrameCount;
            public bool Extended;
            public readonly byte[] RawDefinition =
                new byte[90];

            public readonly List<Frame> Frames =
                new List<Frame>();
        }

        readonly byte[] fileBytes;

        readonly uint[] wallDirectory =
            new uint[256];

        readonly List<SequenceCache> caches =
            new List<SequenceCache>();

        readonly Dictionary<uint, byte> cacheByStream =
            new Dictionary<uint, byte>();

        readonly Dictionary<byte, byte> cacheByWallId =
            new Dictionary<byte, byte>();

        public string SourcePath { get; private set; }

        public IReadOnlyList<SequenceCache> Caches
        {
            get { return caches; }
        }

        public OriginalImgWallRuntime(
            string path,
            List<OriginalRendererCore.Vec> vectors)
        {
            SourcePath =
                Path.GetFullPath(path);

            fileBytes =
                File.ReadAllBytes(path);

            if (fileBytes.Length < 0xBC00)
            {
                throw new InvalidDataException(
                    "IMG file is shorter than the checked directory+SEQDEF region.");
            }

            for (int i = 0;
                i < 256;
                i++)
            {
                wallDirectory[i] =
                    ReadUInt32(
                        i * 4);
            }

            BuildWallCaches(
                vectors);
        }

        void BuildWallCaches(
            List<OriginalRendererCore.Vec> vectors)
        {
            foreach (OriginalRendererCore.Vec vec
                in vectors)
            {
                uint stream =
                    wallDirectory[
                        vec.WallId];

                if (stream == 0)
                {
                    throw new InvalidDataException(
                        "Renderable VEC wall ID " +
                        vec.WallId +
                        " has a null IMG wall-directory offset.");
                }

                if (!cacheByStream.TryGetValue(
                    stream,
                    out byte cacheIndex))
                {
                    if (caches.Count >= 256)
                    {
                        throw new InvalidOperationException(
                            "Wall sequence cache exceeds byte selector range.");
                    }

                    cacheIndex =
                        (byte)caches.Count;

                    SequenceCache cache =
                        LoadLowWallSequence(
                            vec.WallId,
                            stream);

                    caches.Add(
                        cache);

                    cacheByStream[
                        stream] =
                        cacheIndex;
                }

                vec.TextureSet =
                    cacheIndex;

                cacheByWallId[
                    vec.WallId] =
                    cacheIndex;
            }
        }

        SequenceCache LoadLowWallSequence(
            byte wallId,
            uint streamOffset)
        {
            int seqOffset =
                0x0800 +
                wallId * 0x5A;

            if (seqOffset + 90 >
                fileBytes.Length)
            {
                throw new InvalidDataException(
                    "Low wall SEQDEF lies outside IMG.");
            }

            SequenceCache cache =
                new SequenceCache();

            cache.SourceWallId =
                wallId;

            cache.StreamOffset =
                streamOffset;

            Array.Copy(
                fileBytes,
                seqOffset,
                cache.RawDefinition,
                0,
                90);

            cache.IntervalMs =
                ReadUInt16(
                    seqOffset);

            cache.FrameCount =
                fileBytes[
                    seqOffset + 2];

            cache.Extended =
                fileBytes[
                    seqOffset + 3] != 0;

            if (cache.FrameCount == 0)
            {
                throw new InvalidDataException(
                    "Renderable wall sequence has zero frame count for wall ID " +
                    wallId);
            }

            uint p =
                streamOffset;

            for (int frame = 0;
                frame < cache.FrameCount;
                frame++)
            {
                Frame decoded =
                    ReadFrame(p);

                cache.Frames.Add(
                    decoded);

                p +=
                    (uint)(
                        10 +
                        decoded.Width *
                        decoded.Height);
            }

            return cache;
        }

        Frame ReadFrame(
            uint fileOffset)
        {
            if (fileOffset >
                    Int32.MaxValue ||
                fileOffset + 10 >
                    fileBytes.Length)
            {
                throw new InvalidDataException(
                    "IMG frame header outside file.");
            }

            int p =
                (int)fileOffset;

            byte width =
                fileBytes[p];

            byte height =
                fileBytes[p + 1];

            if (width == 0 ||
                height == 0)
            {
                throw new InvalidDataException(
                    "IMG frame has zero dimensions.");
            }

            int size =
                width * height;

            if ((long)p + 10 + size >
                fileBytes.Length)
            {
                throw new InvalidDataException(
                    "IMG frame payload outside file.");
            }

            Frame frame =
                new Frame();

            frame.FileOffset =
                fileOffset;

            frame.Width =
                width;

            frame.Height =
                height;

            Array.Copy(
                fileBytes,
                p,
                frame.Header,
                0,
                10);

            frame.Pixels =
                new byte[
                    width,
                    height];

            int src =
                p + 10;

            // File pixels are x-major / column-major.
            for (int x = 0;
                x < width;
                x++)
            {
                for (int y = 0;
                    y < height;
                    y++)
                {
                    frame.Pixels[x, y] =
                        fileBytes[src++];
                }
            }

            return frame;
        }

        public bool TryGetCacheIndexForWallId(
            byte wallId,
            out byte cacheIndex)
        {
            return cacheByWallId.TryGetValue(
                wallId,
                out cacheIndex);
        }

        public SequenceCache CacheFor(
            OriginalRendererCore.Vec vec)
        {
            if (vec.TextureSet >=
                caches.Count)
            {
                throw new InvalidOperationException(
                    "VEC wall sequence cache selector is invalid.");
            }

            return
                caches[
                    vec.TextureSet];
        }

        /// <summary>
        /// 66B0 frame selection: clamp frame byte to count-1, then select
        /// frame record at frameIndex*10 from the sequence's frame table.
        /// </summary>
        public Frame SelectFrame(
            OriginalRendererCore.Vec vec)
        {
            SequenceCache cache =
                CacheFor(vec);

            if (vec.AnimationFrame >=
                cache.FrameCount)
            {
                vec.AnimationFrame =
                    (byte)(
                        cache.FrameCount -
                        1);
            }

            return
                cache.Frames[
                    vec.AnimationFrame];
        }

        /// <summary>
        /// Shared interval write, matching the 51AE cache behavior used by
        /// SPECIAL1 and other runtime animation controls.
        /// </summary>
        public bool SetIntervalForWallId(
            byte wallId,
            ushort intervalMs)
        {
            if (!cacheByWallId.TryGetValue(
                wallId,
                out byte index))
            {
                return false;
            }

            caches[index].IntervalMs =
                intervalMs;

            return true;
        }

        /// <summary>
        /// FUN_1010_65A6 called after rendering a visible wall span.
        ///
        /// randomByte must be an original-RNG-compatible provider only when
        /// an extended SEQDEF branch actually needs a new selector.
        /// </summary>
        public void UpdateAfterVisibleSpan(
            OriginalRendererCore.Vec vec,
            uint nowMs,
            Func<byte> randomByte,
            Action<OriginalRendererCore.Vec> explosionComplete)
        {
            SequenceCache cache =
                CacheFor(vec);

            // 66B0 only calls 65A6 when cache interval != 0.
            if (cache.IntervalMs == 0)
            {
                return;
            }

            if (nowMs < vec.RuntimeTimer)
            {
                return;
            }

            vec.AnimationFrame++;

            if (vec.RenderClass == 0x2F)
            {
                vec.AnimationFrame = 0;
            }
            else if (vec.RenderClass == 0x07)
            {
                if (vec.AnimationFrame == 1)
                {
                    vec.AnimationFrame--;
                }
                else if (vec.AnimationFrame >=
                    cache.FrameCount)
                {
                    vec.AnimationFrame =
                        (byte)(
                            cache.FrameCount -
                            1);
                }
            }
            else if (vec.RenderClass == 0x2D)
            {
                if (vec.AnimationFrame >=
                    cache.FrameCount)
                {
                    if (explosionComplete != null)
                    {
                        explosionComplete(vec);
                    }

                    vec.AnimationFrame =
                        (byte)(
                            cache.FrameCount -
                            1);
                }
            }
            else if (!cache.Extended)
            {
                if (vec.AnimationFrame >=
                    cache.FrameCount)
                {
                    vec.AnimationFrame = 0;
                }
            }
            else
            {
                ushort branch =
                    ReadUInt16(
                        cache.RawDefinition,
                        4 +
                        vec.AnimationAux * 2);

                int start =
                    branch & 0xFF;

                int length =
                    branch >> 8;

                if (vec.AnimationFrame >=
                    start + length)
                {
                    if (randomByte == null)
                    {
                        throw new InvalidOperationException(
                            "Extended wall SEQDEF needs an original-compatible random-byte provider.");
                    }

                    do
                    {
                        vec.AnimationAux =
                            (byte)(
                                randomByte() &
                                7);

                        branch =
                            ReadUInt16(
                                cache.RawDefinition,
                                4 +
                                vec.AnimationAux * 2);

                        start =
                            branch & 0xFF;

                        length =
                            branch >> 8;
                    }
                    while (length == 0);

                    vec.AnimationFrame =
                        (byte)start;
                }
            }

            vec.RuntimeTimer =
                unchecked(
                    nowMs +
                    cache.IntervalMs);
        }

        public void CompleteExplodingWall(
            OriginalRendererCore.Vec vec,
            OriginalMapTables map,
            List<OriginalRendererCore.Vec> allVectors)
        {
            // FUN_1018_3C0C begins by locating same-class VECs over the
            // current vector bounds, clears VEC render bit 0 and clears
            // the wall ID along the affected horizontal/vertical trace.
            // This compact port performs those externally visible writes.

            OriginalWallRuntime.GetVecBounds(
                vec,
                out int minX,
                out int minY,
                out int maxX,
                out int maxY);

            foreach (OriginalRendererCore.Vec candidate
                in allVectors)
            {
                if (candidate.RenderClass !=
                    vec.RenderClass)
                {
                    continue;
                }

                OriginalWallRuntime.GetVecBounds(
                    candidate,
                    out int cminX,
                    out int cminY,
                    out int cmaxX,
                    out int cmaxY);

                bool overlaps =
                    cminX <= maxX &&
                    cmaxX >= minX &&
                    cminY <= maxY &&
                    cmaxY >= minY;

                if (overlaps)
                {
                    candidate.Flags &=
                        unchecked((byte)~0x01);
                }
            }

            if (minX == maxX)
            {
                for (int y = minY;
                    y < maxY;
                    y++)
                {
                    if (minX >= 0 &&
                        minX < 64 &&
                        y >= 0 &&
                        y < 64)
                    {
                        map.WallId[
                            minX,
                            y] = 0;
                    }
                }
            }
            else
            {
                for (int x = minX;
                    x < maxX;
                    x++)
                {
                    if (x >= 0 &&
                        x < 64 &&
                        minY >= 0 &&
                        minY < 64)
                    {
                        map.WallId[
                            x,
                            minY] = 0;
                    }
                }
            }
        }

        ushort ReadUInt16(
            int offset)
        {
            return
                (ushort)(
                    fileBytes[offset] |
                    (fileBytes[offset + 1] << 8));
        }

        uint ReadUInt32(
            int offset)
        {
            return
                (uint)(
                    fileBytes[offset] |
                    (fileBytes[offset + 1] << 8) |
                    (fileBytes[offset + 2] << 16) |
                    (fileBytes[offset + 3] << 24));
        }

        static ushort ReadUInt16(
            byte[] bytes,
            int offset)
        {
            return
                (ushort)(
                    bytes[offset] |
                    (bytes[offset + 1] << 8));
        }
    }
}