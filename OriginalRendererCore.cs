using System;

namespace Nitemare3D
{
    /// <summary>
    /// Portable arithmetic/data core recovered from the original Nitemare 3D
    /// Win16 VEC/span renderer.
    ///
    /// This file intentionally contains only instruction-backed low-level
    /// behavior. Map-to-VEC special-class policy and full E798 traversal are
    /// kept outside this core until their OpenNitemare3D data mapping is wired.
    /// </summary>
    public static class OriginalRendererCore
    {
        public const int ScreenWidth = 320;
        public const int ScreenHeight = 200;

        public const int ViewportWidth = 304;
        public const int ViewportHeight = 152;

        public const int ViewLeft = 8;
        public const int ViewRight = 311;
        public const int ViewTop = 4;
        public const int ViewBottom = 155;

        public const int CenterX = 160;
        public const int CenterY = 80;
        public const int CenterYQ4 = 1280;

        public const int NearDepthQ10 = 0x4000;

        public const int MaxVectors = 1000;
        public const int MaxOrientationList = 333;
        public const int MaxSpans = 50;

        public sealed class Vec
        {
            // Recovered 28-byte VEC semantic fields.
            public byte WallId;
            public sbyte TextureOffset;
            public byte AnimationAux;
            public byte AnimationFrame;
            public byte TextureSet;
            public byte Flags;
            public byte RenderClass;
            public byte Orientation;
            public uint RuntimeTimer;

            public short X1;
            public short Y1;
            public short X2;
            public short Y2;

            public short ScreenX1;
            public short ProjectedY1Q4;
            public short ScreenX2;
            public short ProjectedY2Q4;

            // OpenNitemare3D bridge metadata, not fields from the original
            // packed 28-byte record.
            public int TextureIndex;
            public long CameraDepth1;
            public long CameraDepth2;
            public long CameraLateral1;
            public long CameraLateral2;
        }

        public sealed class Span
        {
            public Vec Owner;
            public short XStart;
            public short YAtStart;
            public short XEnd;
            public short YAtEnd;
            public int YStep16_16;
            public uint YAccumulator16_16;
        }

        public sealed class WallSamplingTables
        {
            public readonly uint[] Step16_16 = new uint[512];
            public readonly byte[] ClippedStartTexel = new byte[512];
            public readonly ushort[] ClippedStartFraction = new ushort[512];
        }

        public sealed class ProjectionConstants
        {
            public int HorizontalScale;
            public long HeightProduct;
            public long VerticalNumerator;
            public long InverseProjectionScale;
        }

        public static ProjectionConstants BuildProjectionConstants(
            int viewportWidth,
            int viewportHeight)
        {
            // FUN_1010_E4B2 / matching DOS family.
            ProjectionConstants p = new ProjectionConstants();

            p.HorizontalScale =
                (int)(((long)0x2EE0 * viewportWidth) / 0x5000);

            p.HeightProduct =
                (long)0x8340 * viewportHeight;

            p.VerticalNumerator =
                p.HeightProduct << 4;

            p.InverseProjectionScale =
                p.VerticalNumerator / p.HorizontalScale;

            return p;
        }

        public static short Wrap16(int value)
        {
            return unchecked((short)value);
        }

        public static short Subtract16(short a, short b)
        {
            return unchecked((short)(
                unchecked((ushort)a) - unchecked((ushort)b)));
        }

        public static short Negate16(short value)
        {
            return unchecked((short)(
                0 - unchecked((ushort)value)));
        }

        /// <summary>
        /// FUN_1018_3564 occupied-column decision.
        /// Equal comparisons do not replace the current owner.
        /// </summary>
        public static bool OwnerConflictReplaces(Vec oldOwner, Vec candidate)
        {
            switch (oldOwner.Orientation)
            {
                case 0:
                    if (candidate.Orientation == 2)
                    {
                        return candidate.X1 > oldOwner.X1 &&
                               candidate.Y1 < oldOwner.Y1;
                    }

                    if (candidate.Orientation == 3)
                    {
                        return candidate.X2 < oldOwner.X2 &&
                               candidate.Y1 < oldOwner.Y1;
                    }

                    return false;

                case 1:
                    if (candidate.Orientation == 2)
                    {
                        return candidate.X1 > oldOwner.X1 &&
                               candidate.Y2 > oldOwner.Y2;
                    }

                    if (candidate.Orientation == 3)
                    {
                        return candidate.X2 < oldOwner.X2 &&
                               candidate.Y2 > oldOwner.Y2;
                    }

                    return false;

                case 2:
                    if (candidate.Orientation == 0)
                    {
                        return candidate.X2 > oldOwner.X2 &&
                               candidate.Y2 < oldOwner.Y2;
                    }

                    if (candidate.Orientation == 1)
                    {
                        return candidate.X2 > oldOwner.X2 &&
                               candidate.Y1 > oldOwner.Y1;
                    }

                    return false;

                case 3:
                    if (candidate.Orientation == 0)
                    {
                        return candidate.X1 < oldOwner.X1 &&
                               candidate.Y2 < oldOwner.Y2;
                    }

                    if (candidate.Orientation == 1)
                    {
                        return candidate.X1 < oldOwner.X1 &&
                               candidate.Y1 > oldOwner.Y1;
                    }

                    return false;

                default:
                    return false;
            }
        }

        /// <summary>
        /// FUN_1010_6152 signed 16.16 span interpolation setup.
        /// </summary>
        public static bool InitializeSpanInterpolation(
            Span span,
            short x1,
            short y1Q4,
            short x2,
            short y2Q4,
            short centerYQ4)
        {
            short dx = Subtract16(x2, x1);
            short dy = Subtract16(y2Q4, y1Q4);

            int step = 0;

            if (dx != 0)
            {
                long numerator = (long)dy * 65536L;
                long quotient = numerator / dx;

                if (quotient < Int32.MinValue ||
                    quotient > Int32.MaxValue)
                {
                    return false;
                }

                step = (int)quotient;
            }

            short yStart = y1Q4;
            short yEnd = dy == 0 ? y1Q4 : y2Q4;

            uint accumulator =
                ((uint)(ushort)Subtract16(y1Q4, centerYQ4)) << 16;

            if (dx != 0 && dy != 0)
            {
                yStart = InterpolateY(span.XStart);
                yEnd = InterpolateY(span.XEnd);

                short startOffset =
                    Subtract16(span.XStart, x1);

                long product =
                    (long)startOffset * step;

                accumulator = unchecked(
                    accumulator + (uint)product);
            }

            span.YAtStart = yStart;
            span.YAtEnd = yEnd;
            span.YStep16_16 = step;
            span.YAccumulator16_16 = accumulator;
            return true;

            short InterpolateY(short x)
            {
                short offset = Subtract16(x, x1);
                long product = (long)offset * dy;
                long value =
                    (long)y1Q4 + product / dx;

                return unchecked((short)(ushort)value);
            }
        }

        /// <summary>
        /// FUN_1010_6422 endpoint/U correction.
        /// </summary>
        public static ushort SelectTextureU(
            Vec vec,
            short screenX,
            short alongWall,
            ushort width)
        {
            bool nearLeft =
                Subtract16(screenX, vec.ScreenX1) < 8;

            bool nearRight =
                Subtract16(vec.ScreenX2, screenX) < 8;

            ushort mask =
                unchecked((ushort)(width - 1));

            if ((!nearLeft && !nearRight) ||
                (vec.Flags & 0x08) != 0)
            {
                return (ushort)(
                    unchecked((ushort)alongWall) & mask);
            }

            short length =
                (vec.Orientation == 0 ||
                 vec.Orientation == 1)
                ? Subtract16(vec.X2, vec.X1)
                : Subtract16(vec.Y2, vec.Y1);

            if (vec.RenderClass == 2 &&
                (vec.Orientation == 0 ||
                 vec.Orientation == 3))
            {
                if (vec.Orientation == 0)
                {
                    if ((nearRight && alongWall < 0) ||
                        screenX == vec.ScreenX2)
                    {
                        return 0;
                    }

                    if ((nearLeft && length <= alongWall) ||
                        screenX == vec.ScreenX1)
                    {
                        alongWall =
                            Subtract16(length, 1);
                    }
                }
                else
                {
                    if ((nearLeft && alongWall >= 0) ||
                        screenX == vec.ScreenX1)
                    {
                        alongWall = -1;
                    }
                    else if (
                        (nearRight &&
                         Negate16(length) > alongWall) ||
                        screenX == vec.ScreenX2)
                    {
                        alongWall =
                            Negate16(length);
                    }
                }
            }
            else if (
                vec.Orientation == 0 ||
                vec.Orientation == 2)
            {
                if ((nearRight && alongWall >= 0) ||
                    screenX == vec.ScreenX2)
                {
                    alongWall = -1;
                }
                else if (
                    (nearLeft &&
                     Negate16(length) > alongWall) ||
                    screenX == vec.ScreenX1)
                {
                    alongWall =
                        Negate16(length);
                }
            }
            else if (
                vec.Orientation == 1 ||
                vec.Orientation == 3)
            {
                if ((nearLeft && alongWall < 0) ||
                    screenX == vec.ScreenX1)
                {
                    alongWall = 0;
                }
                else if (
                    (nearRight &&
                     length <= alongWall) ||
                    screenX == vec.ScreenX2)
                {
                    alongWall =
                        Subtract16(length, 1);
                }
            }

            return (ushort)(
                unchecked((ushort)alongWall) & mask);
        }

        /// <summary>
        /// FUN_1010_2930 + raw 1010:2960 table generation.
        /// </summary>
        public static WallSamplingTables MakeWallSamplingTables(
            ushort viewportHeight)
        {
            WallSamplingTables tables =
                new WallSamplingTables();

            for (uint n = 1; n <= 511; n++)
            {
                tables.Step16_16[n] =
                    0x400000u / n;

                uint delta =
                    n > viewportHeight
                    ? n - viewportHeight
                    : 0u;

                uint numerator =
                    (delta * 64u) << 16;

                uint clippedStart =
                    (numerator / n) >> 1;

                tables.ClippedStartTexel[n] =
                    (byte)((clippedStart >> 16) & 0xff);

                tables.ClippedStartFraction[n] =
                    (ushort)(clippedStart & 0xffff);
            }

            return tables;
        }

        public static uint ClippedStart16_16(
            WallSamplingTables tables,
            int height)
        {
            if (height < 0 ||
                height >= tables.Step16_16.Length)
            {
                return 0;
            }

            return
                ((uint)tables.ClippedStartTexel[height] << 16) |
                tables.ClippedStartFraction[height];
        }

        public static bool DrawIndexedColumn(
            byte[,] framebuffer,
            int x,
            int firstY,
            int pixelCount,
            byte[] textureColumn,
            uint source16_16,
            uint step16_16,
            byte[] paletteRemap)
        {
            if (framebuffer == null ||
                textureColumn == null ||
                textureColumn.Length < 64 ||
                x < 0 ||
                x >= framebuffer.GetLength(0) ||
                firstY < 0 ||
                pixelCount < 0 ||
                firstY + pixelCount >
                    framebuffer.GetLength(1))
            {
                return false;
            }

            uint source = source16_16;

            for (int y = 0; y < pixelCount; y++)
            {
                if ((source >> 16) >= 64)
                {
                    return false;
                }

                source = unchecked(
                    source + step16_16);
            }

            source = source16_16;

            for (int y = 0; y < pixelCount; y++)
            {
                byte texel =
                    textureColumn[source >> 16];

                framebuffer[x, firstY + y] =
                    paletteRemap == null
                    ? texel
                    : paletteRemap[texel];

                source = unchecked(
                    source + step16_16);
            }

            return true;
        }

        /// <summary>
        /// Original orientation-list comparator at 1018:33BA:
        /// vertical VECs sort by X1, horizontal VECs by Y1.
        /// </summary>
        public static int CompareOrientationList(
            Vec first,
            Vec second)
        {
            int a;
            int b;

            if (first.X1 == first.X2)
            {
                a = first.X1;
                b = second.X1;
            }
            else
            {
                a = first.Y1;
                b = second.Y1;
            }

            if (a < b)
            {
                return -1;
            }

            if (a > b)
            {
                return 1;
            }

            return 0;
        }
    }
}