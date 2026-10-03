namespace Nitemare3D
{
    /// <summary>
    /// Exact MAP-cell occupancy semantics from Win16 FUN_1010_7A44
    /// (GUARD state 0x13 scripted displacement).
    ///
    /// This path is intentionally separate from ordinary movement collision:
    /// it tests only MAP byte +1 (object id), permits movement within the current
    /// cell, rejects the player's current cell, and moves the object byte when
    /// the actor crosses into a new cell.
    /// </summary>
    public static class OriginalState13MapBridge
    {
        public static bool DestinationAllowsMove(
            OriginalMapTables map,
            int oldTileX,
            int oldTileY,
            int targetTileX,
            int targetTileY,
            int playerTileX,
            int playerTileY)
        {
            if (map == null)
                return false;

            if (targetTileX < 0 || targetTileY < 0 ||
                targetTileX >= OriginalRuntime.MapWidth ||
                targetTileY >= OriginalRuntime.MapHeight)
            {
                return false;
            }

            // FUN_7A44 accepts a candidate that still belongs to the actor's
            // currently bound MAP cell without testing the cell object byte.
            if (targetTileX == oldTileX &&
                targetTileY == oldTileY)
            {
                return true;
            }

            // New destination cell must have an empty MAP object byte.
            if (map.ObjectId[targetTileX, targetTileY] != 0)
                return false;

            // Destination may not be the player's current MAP cell.
            if (targetTileX == playerTileX &&
                targetTileY == playerTileY)
            {
                return false;
            }

            return true;
        }

        public static bool CommitCellTransfer(
            OriginalMapTables map,
            int oldTileX,
            int oldTileY,
            int newTileX,
            int newTileY)
        {
            if (map == null)
                return false;

            if (oldTileX < 0 || oldTileY < 0 ||
                oldTileX >= OriginalRuntime.MapWidth ||
                oldTileY >= OriginalRuntime.MapHeight ||
                newTileX < 0 || newTileY < 0 ||
                newTileX >= OriginalRuntime.MapWidth ||
                newTileY >= OriginalRuntime.MapHeight)
            {
                return false;
            }

            if (oldTileX == newTileX &&
                oldTileY == newTileY)
            {
                return true;
            }

            // FUN_7A44 copies old cell +1 to destination +1, then clears source.
            map.ObjectId[newTileX, newTileY] =
                map.ObjectId[oldTileX, oldTileY];
            map.ObjectId[oldTileX, oldTileY] = 0;
            return true;
        }
    }
}
