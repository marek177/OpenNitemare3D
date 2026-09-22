namespace Nitemare3D
{
    /// <summary>
    /// Constants recovered from the original Nitemare-3D data and the audited
    /// NITE3W.EXE V1.10 runtime. These values are intentionally kept separate
    /// from port-specific tuning values so original behavior is not confused
    /// with OpenNitemare3D abstractions.
    ///
    /// Evidence and caveats:
    /// https://github.com/marek177/Nitemare3d-reversed
    /// </summary>
    public static class OriginalRuntimeFacts
    {
        // MAP / world geometry.
        public const int MapWidth = 64;
        public const int MapHeight = 64;
        public const int MapCellBytes = 2;
        public const int MapPayloadBytes = MapWidth * MapHeight * MapCellBytes; // 8192
        public const int SuppliedMapHeaderBytes = 514;
        public const int WorldUnitsPerTile = 64;
        public const int TileCenterOffset = 32;
        public const int PlayerCollisionHalfExtent = 27;

        // Original indexed framebuffer / normal 3-D viewport.
        public const int FramebufferWidth = 320;
        public const int FramebufferHeight = 200;
        public const int ViewportX = 8;
        public const int ViewportY = 4;
        public const int ViewportWidth = 304;
        public const int ViewportHeight = 152;
        public const int ViewportCenterX = 160;
        public const int ViewportCenterY = 80;

        // Verified NITE3W.EXE V1.10 fixed runtime capacities.
        public const int MaxDoors = 64;
        public const int DoorRecordBytes = 22;
        public const int MaxPanels = 32;
        public const int PanelRecordBytes = 22;
        public const int MaxPushes = 12;
        public const int PushRecordBytes = 6;
        public const int MaxObjects = 350;
        public const int ObjectRecordBytes = 28;
        public const int MaxGuards = 100;
        public const int GuardRecordBytes = 26;
        public const int MaxVectors = 1000;
        public const int VectorRecordBytes = 28;
        public const int VectorListEntriesPerOrientation = 333;
        public const int VectorListOrientations = 4;
        public const int MaxVisibleWallSpans = 50;
        public const int VisibleWallSpanRecordBytes = 20;
        public const int MaxProjectedSprites = 100;
        public const int ProjectedSpriteRecordBytes = 18;

        // Player / guard values recovered from the original Windows runtime.
        public const int PlayerNormalMaxHealth = 100;
        public const int GuardInitialStrength = 255;
        public const int GuardStateCount = 22; // 0x00..0x15
        public const int GuardPainState = 0x15;

        // Original input-mask bits used by DEMO/runtime handling.
        public const ushort InputForward = 0x0002;
        public const ushort InputBackward = 0x0004;
        public const ushort InputTurnA = 0x0008;
        public const ushort InputTurnB = 0x0010;
        public const ushort InputFast = 0x0020;
        public const ushort InputFineStep = 0x0040;
        public const ushort InputFire = 0x0080;
        public const ushort InputStrafeModifier = 0x0100;
        public const ushort InputUse = 0x0200;

        // DEMO stream.
        public const int DemoHeaderBytes = 6;
        public const int DemoRecordBytes = 8;

        // Win16 save/config facts; DOS formats must be confirmed separately.
        public const int WindowsConfigSaveBytes = 20;
        public const int WindowsUserSaveRecordBytes = 55015; // 0xD6E7

        // Normal ammo pickup/display cap recovered from NITE3W.EXE.
        public const int NormalAmmoCap = 100;
    }
}
