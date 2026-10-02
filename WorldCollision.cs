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
            return CanOccupy(position, PlayerHalfExtentTiles, ignore);
        }

        public static bool CanOccupy(Vec2 position, float halfExtent, Entity ignore)
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

            for (int x = minTileX; x <= maxTileX; x++)
            {
                for (int y = minTileY; y <= maxTileY; y++)
                {
                    var tile = Level.tilemap[x, y];
                    if (tile == null || tile.obstacle)
                    {
                        return false;
                    }
                }
            }

            // Preserve the historical port's "solid entity occupies a cell"
            // behavior, but apply it to the player's complete AABB instead of
            // only the next integer sample point.
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
