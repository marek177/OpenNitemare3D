namespace Nitemare3D
{
    /// <summary>
    /// Legacy raw WALLS door-family selector helper retained for door-facing
    /// bridge code/tests. It is NOT GUARD+0x0E / DAT_4C1C; those are class-0x44
    /// AREA ids recovered through FUN_247A.
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
