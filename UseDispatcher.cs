using System;

namespace Nitemare3D
{
    /// <summary>
    /// First production integration point for the recovered edge-triggered USE path.
    /// Exact per-wall side effects remain delegated to Tile/Entity handlers until the
    /// full original wall-class dispatcher is integrated.
    /// </summary>
    public static class UseDispatcher
    {
        public static bool TryUseAdjacent(Player player)
        {
            int cellX = (int)MathF.Floor(player.position.X);
            int cellY = (int)MathF.Floor(player.position.Y);

            int stepX = 0;
            int stepY = 0;

            if (MathF.Abs(player.direction.X) >= MathF.Abs(player.direction.Y))
            {
                stepX = player.direction.X >= 0 ? 1 : -1;
            }
            else
            {
                stepY = player.direction.Y >= 0 ? 1 : -1;
            }

            int targetX = cellX + stepX;
            int targetY = cellY + stepY;

            if (targetX < 0 || targetY < 0 ||
                targetX >= OriginalRuntime.MapWidth ||
                targetY >= OriginalRuntime.MapHeight)
            {
                return false;
            }

            bool handled = false;

            // Tile handlers cover curtains and future door/special-wall implementations.
            // Calling the base Tile.OnUse is intentionally harmless.
            var tile = Level.tilemap[targetX, targetY];
            if (tile != null)
            {
                tile.OnUse();
            }

            // Entity-based interactables include the historical HiddenPanel implementation.
            foreach (var entity in Entity.entities)
            {
                if (!(entity is IUsable usable))
                {
                    continue;
                }

                int ex = (int)MathF.Floor(entity.position.X);
                int ey = (int)MathF.Floor(entity.position.Y);

                if (ex == targetX && ey == targetY)
                {
                    usable.OnUse();
                    handled = true;
                }
            }

            return handled;
        }
    }
}
