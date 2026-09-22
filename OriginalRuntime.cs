namespace Nitemare3D
{
    // Clean-room constants synchronized with the original NITE3W.EXE reverse engineering.
    // Values marked here have direct EXE/data/save evidence in Nitemare3d-reversed.
    public static class OriginalRuntime
    {
        public const int MapWidth = 64, MapHeight = 64, WorldUnitsPerTile = 64;
        public const int MapHeaderBytes = 514, MapLevelBytes = 8192;
        public const int PlayerMaxHealth = 100, PlayerCollisionHalfExtent = 27;
        public const ushort InputFire = 0x0080, InputStrafe = 0x0100, InputUse = 0x0200;
        public const int MaxDoors = 64, MaxPanels = 32, MaxPushables = 12;
        public const int MaxObjects = 350, ObjectRuntimeStride = 28;
        public const int MaxGuards = 100, GuardRuntimeStride = 26;
        public const int MaxVectors = 1000;
        public const int FramebufferWidth = 320, FramebufferHeight = 200;
        public const int ViewportX = 8, ViewportY = 4, ViewportWidth = 304, ViewportHeight = 152;
        public const byte TransparentPaletteIndex = 0x29;
        public const byte GuardPainState = 0x15;

        public static readonly int[] GuardScoreByObjectClass = {
            25,75,50,100,250,150,200,100,100,0,150,150,200,-1000,1000,
            100,200,0,25,100,100,250,250,200,50
        };

        public static int GuardScore(byte objectClass)
        {
            return objectClass >= 0x08 && objectClass <= 0x20
                ? GuardScoreByObjectClass[objectClass - 0x08] : 0;
        }
    }
}
