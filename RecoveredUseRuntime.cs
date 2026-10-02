using System;

namespace Nitemare3D
{
    // USE_INTERACTION_RE.md: seg3:98ED edge latch; seg3:1A22, DS:009A table.
    public sealed class RecoveredUseRuntime
    {
        bool previousUse;
        public bool Press(bool down)
        {
            bool rising = down && !previousUse;
            previousUse = down;
            return rising;
        }

        // Octants N, NE, E, SE, S, SW, W, NW. Check coordinates independently
        // so east/west cannot wrap into the next row at a map boundary.
        public static bool TryTarget(int x, int y, int octant, out int targetX, out int targetY)
        {
            targetX = x;
            targetY = y;
            if (octant < 0 || octant > 7) throw new ArgumentOutOfRangeException(nameof(octant));
            if (x < 0 || x >= 64 || y < 0 || y >= 64) return false;
            switch (octant)
            {
                case 0: case 7: --targetY; break;
                case 1: case 2: ++targetX; break;
                case 3: case 4: ++targetY; break;
                case 5: case 6: --targetX; break;
            }
            return targetX >= 0 && targetX < 64 && targetY >= 0 && targetY < 64;
        }
    }
}
