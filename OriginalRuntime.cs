namespace Nitemare3D
{
    /// <summary>
    /// Clean-room constants synchronized with direct Nitemare-3D executable/data analysis.
    ///
    /// NITE3W.EXE-specific values are kept here as original-runtime reference values;
    /// DOS builds must be cross-checked independently before those values are called DOS facts.
    /// See marek177/Nitemare3d-reversed for evidence and confidence boundaries.
    /// </summary>
    public static class OriginalRuntime
    {
        // MAP / world geometry.
        public const int MapWidth = 64;
        public const int MapHeight = 64;
        public const int MapCellBytes = 2;
        public const int MapHeaderBytes = 514;
        public const int MapLevelBytes = MapWidth * MapHeight * MapCellBytes; // 8192
        public const int WorldUnitsPerTile = 64;
        public const int TileCenterOffset = 32;

        // Player.
        public const int PlayerMaxHealth = 100;
        public const int PlayerCollisionHalfExtent = 27;

        // Input mask recovered from DEMO/runtime handling.
        public const ushort InputForward = 0x0002;
        public const ushort InputBackward = 0x0004;
        public const ushort InputTurnA = 0x0008;
        public const ushort InputTurnB = 0x0010;
        public const ushort InputFast = 0x0020;
        public const ushort InputFineStep = 0x0040;
        public const ushort InputFire = 0x0080;
        public const ushort InputStrafe = 0x0100;
        public const ushort InputUse = 0x0200;

        // Fixed original Win16 runtime capacities / strides.
        public const int MaxDoors = 64;
        public const int DoorRuntimeStride = 22;
        public const int MaxPanels = 32;
        public const int PanelRuntimeStride = 22;
        public const int MaxPushables = 12;
        public const int PushRuntimeStride = 6;
        public const int MaxObjects = 350;
        public const int ObjectRuntimeStride = 28;
        public const int MaxGuards = 100;
        public const int GuardRuntimeStride = 26;
        public const int MaxVectors = 1000;
        public const int VectorRuntimeStride = 28;
        public const int VectorListOrientations = 4;
        public const int VectorListEntriesPerOrientation = 333;
        public const int MaxVisibleWallSpans = 50;
        public const int VisibleWallSpanStride = 20;
        public const int MaxProjectedSprites = 100;
        public const int ProjectedSpriteStride = 18;

        // Indexed framebuffer / normal 3-D viewport.
        public const int FramebufferWidth = 320;
        public const int FramebufferHeight = 200;
        public const int ViewportX = 8;
        public const int ViewportY = 4;
        public const int ViewportWidth = 304;
        public const int ViewportHeight = 152;
        public const int ViewportCenterX = 160;
        public const int ViewportCenterY = 80;
        public const byte TransparentPaletteIndex = 0x29;

        // GUARD runtime.
        public const int GuardInitialStrength = 255;
        public const int GuardStateCount = 22; // 0x00..0x15
        public const byte GuardPainState = 0x15;
        public const byte DraculaPhase1Class = 0x11;
        public const byte DraculaBatPhase2Class = 0x14;

        // DEMO stream.
        public const int DemoHeaderBytes = 6;
        public const int DemoRecordBytes = 8;

        // Win16 save/config facts. Do not assume DOS uses these unchanged.
        public const int WindowsConfigSaveBytes = 20;
        public const int WindowsUserSaveRecordBytes = 55015; // 0xD6E7

        // Normal ammo pickup/display cap recovered from NITE3W.EXE.
        public const int NormalAmmoCap = 100;

        // Original score switch for OBJECT classes 0x08..0x20.
        public static readonly int[] GuardScoreByObjectClass =
        {
            25, 75, 50, 100, 250, 150, 200, 100, 100, 0,
            150, 150, 200, -1000, 1000, 100, 200, 0, 25, 100,
            100, 250, 250, 200, 50
        };

        public static int GuardScore(byte objectClass)
        {
            return objectClass >= 0x08 && objectClass <= 0x20
                ? GuardScoreByObjectClass[objectClass - 0x08]
                : 0;
        }
    }
}
