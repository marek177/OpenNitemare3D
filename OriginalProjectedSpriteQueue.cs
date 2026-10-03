namespace Nitemare3D
{
    /// <summary>
    /// Portable model of the 100 x 18-byte projected-sprite slot allocator used
    /// by CC7C/6914. Slot byte 0 is 1 when free and 0 when active.
    /// </summary>
    public static class OriginalProjectedSpriteQueue
    {
        public const int SlotCount = OriginalRuntime.MaxProjectedSprites;

        public static int PreferredSlotFromBaseline(int baselineRow)
        {
            int row =
                baselineRow <
                    OriginalRendererCore.ViewBottom - 1
                ? baselineRow
                : OriginalRendererCore.ViewBottom - 1;

            return row - OriginalRendererCore.CenterY;
        }

        public static int FindFreeSlot(
            bool[] occupied,
            int baselineRow)
        {
            if (occupied == null ||
                occupied.Length < SlotCount)
            {
                return -1;
            }

            int preferred =
                PreferredSlotFromBaseline(
                    baselineRow);

            if (preferred < 0)
                preferred = 0;
            if (preferred >= SlotCount)
                preferred = SlotCount - 1;

            // CC7C first walks toward the horizon while the slot is active.
            for (int slot = preferred;
                slot >= 0;
                slot--)
            {
                if (!occupied[slot])
                    return slot;
            }

            // If that side is full, restart just below the projected baseline
            // and walk toward the viewport bottom.
            int secondRow =
                baselineRow - 1;

            if (secondRow >
                OriginalRendererCore.ViewBottom - 1)
            {
                secondRow =
                    OriginalRendererCore.ViewBottom - 1;
            }

            int second =
                secondRow -
                OriginalRendererCore.CenterY;

            if (second < 0)
                second = 0;

            int maxNormalViewportSlot =
                OriginalRendererCore.ViewBottom -
                OriginalRendererCore.CenterY;

            if (maxNormalViewportSlot >= SlotCount)
                maxNormalViewportSlot = SlotCount - 1;

            for (int slot = second;
                slot <= maxNormalViewportSlot;
                slot++)
            {
                if (!occupied[slot])
                    return slot;
            }

            return -1;
        }

        public static ushort WallVisibilityQ4FromPerpendicularDistance(
            double distanceTiles)
        {
            if (distanceTiles <= 0)
                return ushort.MaxValue;

            long depthQ10 =
                (long)System.Math.Round(
                    distanceTiles *
                    OriginalRuntime.WorldUnitsPerTile *
                    1024.0);

            if (depthQ10 < OriginalRendererCore.NearDepthQ10)
                depthQ10 = OriginalRendererCore.NearDepthQ10;

            var constants =
                OriginalRendererCore.BuildProjectionConstants(
                    OriginalRendererCore.ViewportWidth,
                    OriginalRendererCore.ViewportHeight);

            long value =
                constants.VerticalNumerator /
                depthQ10 +
                OriginalRendererCore.CenterYQ4;

            if (value < 0)
                return 0;
            if (value > ushort.MaxValue)
                return ushort.MaxValue;

            return (ushort)value;
        }

        public static uint SpriteSourceStep16_16(
            int frameHeight,
            int projectedTop,
            int projectedBottom)
        {
            int screenHeight =
                projectedBottom -
                projectedTop +
                1;

            if (frameHeight <= 0 ||
                screenHeight <= 0)
            {
                return 0;
            }

            return (uint)(
                ((ulong)frameHeight << 16) /
                (ulong)screenHeight);
        }

        public static int SpriteSourceCoordinate(
            int screenCoordinate,
            int projectedStart,
            uint sourceStep16_16)
        {
            if (screenCoordinate <= projectedStart)
                return 0;

            ulong delta =
                (ulong)(
                    screenCoordinate -
                    projectedStart);

            return (int)(
                (delta *
                 sourceStep16_16) >>
                16);
        }

        public static bool PassesThreeColumnWallGate(
            ushort[] wallVisibilityQ4,
            int left,
            int center,
            int right,
            ushort projectedYQ4)
        {
            if (wallVisibilityQ4 == null ||
                wallVisibilityQ4.Length <
                    OriginalRendererCore.ScreenWidth)
            {
                return false;
            }

            return Probe(
                       wallVisibilityQ4,
                       left,
                       projectedYQ4) ||
                   Probe(
                       wallVisibilityQ4,
                       center,
                       projectedYQ4) ||
                   Probe(
                       wallVisibilityQ4,
                       right,
                       projectedYQ4);
        }

        public static bool ColumnPassesWall(
            ushort[] wallVisibilityQ4,
            int column,
            ushort projectedYQ4,
            bool bypassWall)
        {
            if (bypassWall)
                return true;

            return Probe(
                wallVisibilityQ4,
                column,
                projectedYQ4);
        }

        static bool Probe(
            ushort[] wallVisibilityQ4,
            int column,
            ushort projectedYQ4)
        {
            // CC7C's raw pointer probes can reach outside the logical viewport
            // for heavily clipped sprites. Clamp to the framebuffer edge here
            // rather than reproducing adjacent-memory reads from Win16.
            if (column < 0)
                column = 0;
            else if (column >= wallVisibilityQ4.Length)
                column = wallVisibilityQ4.Length - 1;

            return
                wallVisibilityQ4[column] <=
                projectedYQ4;
        }
    }
}
