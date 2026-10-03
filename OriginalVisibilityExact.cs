using System;
using System.Collections.Generic;

namespace Nitemare3D
{
    /// <summary>
    /// Literal semantic port of Win16 FUN_1010_87B6 +
    /// FUN_1018_3940 for wall-vector visibility traversal.
    ///
    /// The four orientation lists are already sorted by FUN_1018_33BA rules.
    /// Projection/column conflict uses the exact E798 and 3564 semantics.
    /// </summary>
    public static class OriginalVisibilityExact
    {
        public sealed class Result
        {
            public int OwnedColumns;
            public short MinX1;
            public short MaxX2;
            public short MinY1;
            public short MaxY2;

            // The original stores expanded bounds after 3940.
            public short ExpandedMinX1;
            public short ExpandedMaxX2;
            public short ExpandedMinY1;
            public short ExpandedMaxY2;
        }

        public static Result BuildOwnerBuffer(
            List<OriginalRendererCore.Vec>[] lists,
            OriginalRendererCore.Vec[] ownerBuffer,
            short playerX,
            short playerY,
            int angleDegrees,
            OriginalTrigQ10 trig,
            OriginalVisibilityOctants octantFlags)
        {
            if (lists == null ||
                lists.Length != 4)
            {
                throw new ArgumentException(
                    "Four orientation lists are required.",
                    nameof(lists));
            }

            if (ownerBuffer == null ||
                ownerBuffer.Length <
                    OriginalRendererCore.ScreenWidth)
            {
                throw new ArgumentException(
                    "320-entry owner buffer is required.",
                    nameof(ownerBuffer));
            }

            if (trig == null)
            {
                throw new ArgumentNullException(
                    nameof(trig));
            }

            if (octantFlags == null)
            {
                throw new ArgumentNullException(
                    nameof(octantFlags));
            }

            for (int x =
                    OriginalRendererCore.ViewLeft;
                x <=
                    OriginalRendererCore.ViewRight;
                x++)
            {
                ownerBuffer[x] = null;
            }

            int remaining =
                OriginalRendererCore.ViewportWidth;

            int angle =
                OriginalTrigQ10.Normalize(
                    angleDegrees);

            int octant =
                angle / 45;

            bool f0 =
                octantFlags.IsEnabled(0, octant);
            bool f1 =
                octantFlags.IsEnabled(1, octant);
            bool f2 =
                octantFlags.IsEnabled(2, octant);
            bool f3 =
                octantFlags.IsEnabled(3, octant);

            // FUN_1010_87B6 steady-state indices are equivalent to the
            // first sorted record whose coordinate is >= the player.
            int i0 =
                LowerBoundY(
                    lists[0],
                    playerY);

            int i1 =
                LowerBoundY(
                    lists[1],
                    playerY);

            int i2 =
                LowerBoundX(
                    lists[2],
                    playerX);

            int i3 =
                LowerBoundX(
                    lists[3],
                    playerX);

            bool complete = false;

            // FUN_1018_3940 initial interleaved pass.
            while (!complete)
            {
                bool progressed = false;

                if (f0 &&
                    i0 < lists[0].Count)
                {
                    progressed = true;

                    complete |=
                        ProcessVector(
                            lists[0][i0],
                            ownerBuffer,
                            playerX,
                            playerY,
                            angle,
                            trig,
                            ref remaining);

                    i0++;
                }

                if (f1 &&
                    i1 > 0)
                {
                    progressed = true;
                    i1--;

                    complete |=
                        ProcessVector(
                            lists[1][i1],
                            ownerBuffer,
                            playerX,
                            playerY,
                            angle,
                            trig,
                            ref remaining);
                }

                if (f2 &&
                    i2 > 0)
                {
                    progressed = true;
                    i2--;

                    complete |=
                        ProcessVector(
                            lists[2][i2],
                            ownerBuffer,
                            playerX,
                            playerY,
                            angle,
                            trig,
                            ref remaining);
                }

                if (f3 &&
                    i3 < lists[3].Count)
                {
                    progressed = true;

                    complete |=
                        ProcessVector(
                            lists[3][i3],
                            ownerBuffer,
                            playerX,
                            playerY,
                            angle,
                            trig,
                            ref remaining);

                    i3++;
                }

                if (!progressed &&
                    !complete)
                {
                    // Original assumes a valid/enclosed map and would otherwise
                    // never leave this loop. Fail explicitly on malformed bridge
                    // input rather than hang the reimplementation.
                    throw new InvalidOperationException(
                        "3940 initial pass exhausted active VEC lists before filling the viewport.");
                }
            }

            short minX =
                Int16.MaxValue;

            short minY =
                Int16.MaxValue;

            short maxX = 0;
            short maxY = 0;

            OriginalRendererCore.Vec previous =
                null;

            for (int x =
                    OriginalRendererCore.ViewLeft;
                x <=
                    OriginalRendererCore.ViewRight;
                x++)
            {
                OriginalRendererCore.Vec owner =
                    ownerBuffer[x];

                if (owner == null)
                {
                    throw new InvalidOperationException(
                        "3940 completed with an empty owner column.");
                }

                if (!Object.ReferenceEquals(
                    owner,
                    previous))
                {
                    if (owner.X1 < minX)
                    {
                        minX = owner.X1;
                    }

                    if (owner.X2 > maxX)
                    {
                        maxX = owner.X2;
                    }

                    if (owner.Y1 < minY)
                    {
                        minY = owner.Y1;
                    }

                    if (owner.Y2 > maxY)
                    {
                        maxY = owner.Y2;
                    }

                    previous = owner;
                }
            }

            // Exact continuation bounds from 3940.

            while (
                f0 &&
                i0 < lists[0].Count)
            {
                OriginalRendererCore.Vec vec =
                    lists[0][i0];

                if (maxY <= vec.Y2)
                {
                    break;
                }

                ProcessVector(
                    vec,
                    ownerBuffer,
                    playerX,
                    playerY,
                    angle,
                    trig,
                    ref remaining);

                i0++;
            }

            i1--;

            while (
                f1 &&
                i1 >= 0)
            {
                OriginalRendererCore.Vec vec =
                    lists[1][i1];

                if (vec.Y1 <= minY)
                {
                    break;
                }

                ProcessVector(
                    vec,
                    ownerBuffer,
                    playerX,
                    playerY,
                    angle,
                    trig,
                    ref remaining);

                i1--;
            }

            i2--;

            while (
                f2 &&
                i2 >= 0)
            {
                OriginalRendererCore.Vec vec =
                    lists[2][i2];

                if (vec.X1 <= minX)
                {
                    break;
                }

                ProcessVector(
                    vec,
                    ownerBuffer,
                    playerX,
                    playerY,
                    angle,
                    trig,
                    ref remaining);

                i2--;
            }

            while (
                f3 &&
                i3 < lists[3].Count)
            {
                OriginalRendererCore.Vec vec =
                    lists[3][i3];

                if (maxX <= vec.X2)
                {
                    break;
                }

                ProcessVector(
                    vec,
                    ownerBuffer,
                    playerX,
                    playerY,
                    angle,
                    trig,
                    ref remaining);

                i3++;
            }

            Result result =
                new Result();

            result.OwnedColumns =
                OriginalRendererCore.ViewportWidth -
                Math.Max(remaining, 0);

            result.MinX1 = minX;
            result.MaxX2 = maxX;
            result.MinY1 = minY;
            result.MaxY2 = maxY;

            result.ExpandedMinX1 =
                OriginalRendererCore.Wrap16(
                    minX - 1);

            result.ExpandedMaxX2 =
                OriginalRendererCore.Wrap16(
                    maxX + 1);

            result.ExpandedMinY1 =
                OriginalRendererCore.Wrap16(
                    minY - 1);

            result.ExpandedMaxY2 =
                OriginalRendererCore.Wrap16(
                    maxY + 1);

            return result;
        }

        static bool ProcessVector(
            OriginalRendererCore.Vec vec,
            OriginalRendererCore.Vec[] ownerBuffer,
            short playerX,
            short playerY,
            int angle,
            OriginalTrigQ10 trig,
            ref int remaining)
        {
            if ((vec.Flags & 0x01) == 0)
            {
                return remaining < 1;
            }

            if (!OriginalProjectionExact.ProjectVec(
                vec,
                playerX,
                playerY,
                angle,
                trig))
            {
                return remaining < 1;
            }

            int left =
                Math.Max(
                    OriginalRendererCore.ViewLeft,
                    (int)vec.ScreenX1);

            int right =
                Math.Min(
                    OriginalRendererCore.ViewRight,
                    (int)vec.ScreenX2);

            if (left > right)
            {
                return remaining < 1;
            }

            for (int x = left;
                x <= right;
                x++)
            {
                OriginalRendererCore.Vec oldOwner =
                    ownerBuffer[x];

                if (oldOwner == null)
                {
                    ownerBuffer[x] = vec;
                    remaining--;
                }
                else if (
                    OriginalRendererCore.OwnerConflictReplaces(
                        oldOwner,
                        vec))
                {
                    ownerBuffer[x] = vec;
                }
            }

            return remaining < 1;
        }

        static int LowerBoundY(
            List<OriginalRendererCore.Vec> list,
            short y)
        {
            int lo = 0;
            int hi = list.Count;

            while (lo < hi)
            {
                int mid =
                    (lo + hi) >> 1;

                if (list[mid].Y1 < y)
                {
                    lo = mid + 1;
                }
                else
                {
                    hi = mid;
                }
            }

            return lo;
        }

        static int LowerBoundX(
            List<OriginalRendererCore.Vec> list,
            short x)
        {
            int lo = 0;
            int hi = list.Count;

            while (lo < hi)
            {
                int mid =
                    (lo + hi) >> 1;

                if (list[mid].X1 < x)
                {
                    lo = mid + 1;
                }
                else
                {
                    hi = mid;
                }
            }

            return lo;
        }
    }
}