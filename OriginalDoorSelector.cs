namespace Nitemare3D
{
    /// <summary>
    /// Recovered selector used by the Win16 guard wake cache.
    /// Only WALLS class-D (DOOR-family) IDs produce a selector.
    /// The value is wallId - 0x70 and is intentionally sparse.
    /// </summary>
    public static class OriginalDoorSelector
    {
        public const byte FirstDoorWallId = 0x70;

        public static bool TryGet(byte wallId, out byte selector)
        {
            if (IsDoorFamilyWall(wallId))
            {
                selector = (byte)(wallId - FirstDoorWallId);
                return true;
            }

            selector = 0;
            return false;
        }

        public static bool IsDoorFamilyWall(byte wallId)
        {
            return
                (wallId >= 0x70 && wallId <= 0x77) ||
                (wallId >= 0x79 && wallId <= 0x80) ||
                (wallId >= 0x82 && wallId <= 0x83) ||
                (wallId >= 0xA3 && wallId <= 0xA6) ||
                (wallId >= 0xAD && wallId <= 0xAE);
        }
    }
}
