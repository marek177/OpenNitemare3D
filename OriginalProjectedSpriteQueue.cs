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

            for (int slot = second;
                slot < SlotCount;
                slot++)
            {
                if (!occupied[slot])
                    return slot;
            }

            return -1;
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
