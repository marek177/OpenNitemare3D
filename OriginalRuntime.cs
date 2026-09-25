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
        public const double RecoveredHorizontalFovDegrees = 80.99;

        // NITE3W 1.10 HUD / automap dispatchers and data.
        public const ushort HudDispatcherSegment = 0x0003;
        public const ushort HudDispatcherOffset = 0xA3B6;
        public const ushort AutomapDispatcherSegment = 0x0003;
        public const ushort AutomapDispatcherOffset = 0xB1A4;
        public const ushort PlayerTileXGlobal = 0x4BF2;
        public const ushort PlayerTileYGlobal = 0x4BF4;
        public const ushort PlayerWorldXGlobal = 0x4BF6;
        public const ushort PlayerWorldYGlobal = 0x4BF8;
        public const ushort PlayerHealthGlobal = 0x4C1D;
        public const ushort EnemyLocatorEnergyGlobal = 0x4C42;
        public const ushort MapClarityEnergyGlobal = 0x4C43;
        public const int AutomapWidth = 64;
        public const int AutomapHeight = 64;
        public const int AutomapBytes = 4096;
        public const int AutomapViewportWidth = 62;
        public const int AutomapViewportHeight = 36;
        public const int AutomapScreenX = 256;
        public const int AutomapScreenY = 162;

        public static int HudPortraitFrame(byte health)
        {
            int hp = health > 100 ? 100 : health;
            return 13 + (hp + 9) / 10;
        }

        // The automap buffer is X-major: x*64+y, unlike MAP's row-major cells.
        public static int AutomapIndex(int cellX, int cellY)
        {
            return cellX * 64 + cellY;
        }

        public static int AutomapOriginX(int playerCellX)
        {
            int value = playerCellX - 31;
            return value < 0 ? 0 : value > 2 ? 2 : value;
        }

        public static int AutomapOriginY(int playerCellY)
        {
            int value = playerCellY - 18;
            return value < 0 ? 0 : value > 28 ? 28 : value;
        }

        public static int AutomapNoisePointCount(byte power)
        {
            if (power == 0 || power > 15) return 0;
            int p = power;
            return 500 / (p * p * p);
        }

        // GUARD runtime.
        public const int GuardInitialStrength = 255;
        public const int GuardStateCount = 22; // 0x00..0x15
        public const byte GuardPerceptionDecisionState = 0x07;
        public const byte GuardLethalContactState = 0x0B;
        public const byte GuardTimedDirectionalMoveState = 0x13;
        public const byte GuardPainState = 0x15;
        public const int GuardResultOctantOffset = 0x12;
        public const int GuardState13MoveXOffset = 0x13;
        public const int GuardState13MoveYOffset = 0x14;
        public const byte DraculaPhase1Class = 0x11;
        public const byte DraculaBatPhase2Class = 0x14;

        // Renderer global arrays/counters recovered from NITE3W 1.10 DS.
        public const ushort WallOwnerTableGlobal = 0x53FE;
        public const ushort WallOcclusionTableGlobal = 0x58FE;
        public const ushort RgbPaletteTableGlobal = 0x5B7E;
        public const ushort VisibleSpanCountGlobal = 0x5E7E;
        public const ushort VisibleSpanArrayGlobal = 0x5E88;
        public const ushort ProjectedSpriteArrayGlobal = 0x6270;
        public const ushort WinGBitmapHandleGlobal = 0x6978;
        public const ushort VectorCountGlobal = 0x7E56;
        public const ushort ObjectCountGlobal = 0x7E58;
        public const ushort GuardCountGlobal = 0x7E5E;

        // IMG/UIF reference facts.
        public const int ImgWallDirectoryOffset = 0x0000;
        public const int ImgObjectDirectoryOffset = 0x0400;
        public const int ImgDirectoryEntries = 256;
        public const int ImgFrameHeaderBytes = 10;
        public const int ImgFirstFrameStreamOffset = 0xBC00;
        public const byte HudImageObjectId = 0xFF;
        public const int HudImageFrameCount = 29;
        public const int UifDirectorySlots = 32;
        public const int UifDirectoryEntryBytes = 6;

        public static int ImgPixelIndex(int x, int y, int height)
        {
            return x * height + y;
        }

        // Menu/save UI.
        public const ushort MenuDispatcherSegment = 0x0004;
        public const ushort MenuDispatcherOffset = 0x27DE;
        public const int MenuActionFirst = 1;
        public const int MenuActionLast = 40;
        public const int MenuTableCount = 14;
        public const int MenuItemCount = 80;
        public const int SaveSlotCount = 10;
        public const int SaveNameMaxChars = 40;
        public const int SaveNameMaxPixels = 179;

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
