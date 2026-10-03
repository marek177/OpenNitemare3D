using System;

namespace Nitemare3D
{
    /// <summary>
    /// Collision helpers expressed in the port's tile-space coordinates.
    /// The original Win16 player collision half-extent is 27 world units
    /// and one map tile is 64 world units.
    /// </summary>
    public static class WorldCollision
    {
        public const float PlayerHalfExtentTiles =
            (float)OriginalRuntime.PlayerCollisionHalfExtent / OriginalRuntime.WorldUnitsPerTile;

        public static bool CanOccupyPlayer(Vec2 position, Entity ignore)
        {
            return CanOccupyCore(
                position,
                PlayerHalfExtentTiles,
                ignore,
                true);
        }

        public static bool CanOccupy(Vec2 position, float halfExtent, Entity ignore)
        {
            return CanOccupyCore(position, halfExtent, ignore, false);
        }

        static bool CanOccupyCore(
            Vec2 position,
            float halfExtent,
            Entity ignore,
            bool applyPlayerWallScripts)
        {
            float minX = position.X - halfExtent;
            float maxX = position.X + halfExtent;
            float minY = position.Y - halfExtent;
            float maxY = position.Y + halfExtent;

            int minTileX = (int)MathF.Floor(minX);
            int maxTileX = (int)MathF.Floor(maxX);
            int minTileY = (int)MathF.Floor(minY);
            int maxTileY = (int)MathF.Floor(maxY);

            if (minTileX < 0 || minTileY < 0 ||
                maxTileX >= OriginalRuntime.MapWidth ||
                maxTileY >= OriginalRuntime.MapHeight)
            {
                return false;
            }

            if (Level.originalMap != null)
            {
                // FUN_84F4 primary phase: hard wall and dynamic-door state.
                for (int x = minTileX; x <= maxTileX; x++)
                {
                    for (int y = minTileY; y <= maxTileY; y++)
                    {
                        if (Level.originalMap.IsPlayerPrimaryWallBlocked84F4(
                                x,
                                y,
                                Level.originalWalls))
                        {
                            return false;
                        }
                    }
                }
            }
            else
            {
                // Fallback only for maps not loaded through OriginalMapTables.
                for (int x = minTileX; x <= maxTileX; x++)
                {
                    for (int y = minTileY; y <= maxTileY; y++)
                    {
                        var tile = Level.tilemap[x, y];
                        if (tile == null || tile.obstacle)
                            return false;
                    }
                }
            }

            // FUN_84F4 performs primary wall/door passability first, then invokes
            // FUN_BFD8 for semantic wall classes 0x47/0x48, before secondary
            // object blocking. Mirror that ordering for the player's AABB probe.
            if (applyPlayerWallScripts && Level.originalMap != null)
            {
                int levelNumber = Game.level + 1;

                for (int x = minTileX; x <= maxTileX; x++)
                {
                    for (int y = minTileY; y <= maxTileY; y++)
                    {
                        byte rawWallId = Level.originalMap.WallId[x, y];
                        byte wallFlags =
                            Level.originalMap.WallProperty[rawWallId];

                        if ((wallFlags & OriginalMapTables.WallScriptTouch) == 0)
                            continue;

                        byte wallClass =
                            Level.originalMap.WallClass[rawWallId];

                        OriginalRuntimeState.ApplyWeaponJamScriptTouchBFD8(
                            Game.episode,
                            levelNumber,
                            wallClass);

                        OriginalRuntimeState.ApplyScriptTouchProgress51A6(
                            Game.episode,
                            levelNumber,
                            wallClass);
                    }
                }
            }

            if (Level.originalMap != null)
            {
                // FUN_84F4 performs special-object callbacks here (property 0x04),
                // then its final collision decision is object property bit 0x02.
                // The special callback itself is recovered separately; preserve
                // the confirmed blocking order now.
                for (int x = minTileX; x <= maxTileX; x++)
                {
                    for (int y = minTileY; y <= maxTileY; y++)
                    {
                        if (Level.originalMap.IsPlayerObjectBlocked84F4(x, y))
                            return false;
                    }
                }
            }

            // Preserve the historical port's solid-entity bridge after the exact
            // MAP wall/object collision tests.
            foreach (var entity in Entity.entities)
            {
                if (ReferenceEquals(entity, ignore) || !entity.hasCollision)
                {
                    continue;
                }

                int ex = (int)MathF.Floor(entity.position.X);
                int ey = (int)MathF.Floor(entity.position.Y);

                if (maxX > ex && minX < ex + 1 &&
                    maxY > ey && minY < ey + 1)
                {
                    return false;
                }
            }

            return true;
        }

        /// <summary>
        /// The original movement path advances in one-world-unit increments and
        /// collision-tests X/Y independently, producing wall sliding. The port uses
        /// tile-space floats, so one recovered world unit is 1/64 of a tile.
        /// </summary>
        public static void MovePlayerWithSliding(Player player, Vec2 delta)
        {
            float maxComponent = MathF.Max(MathF.Abs(delta.X), MathF.Abs(delta.Y));
            int steps = Math.Max(1, (int)MathF.Ceiling(
                maxComponent * OriginalRuntime.WorldUnitsPerTile));

            float stepX = delta.X / steps;
            float stepY = delta.Y / steps;

            for (int i = 0; i < steps; i++)
            {
                var candidateX = new Vec2(player.position.X + stepX, player.position.Y);
                if (CanOccupyPlayer(candidateX, player))
                {
                    player.position.X = candidateX.X;
                }

                var candidateY = new Vec2(player.position.X, player.position.Y + stepY);
                if (CanOccupyPlayer(candidateY, player))
                {
                    player.position.Y = candidateY.Y;
                }
            }
        }
    }
}
