namespace Nitemare3D
{
    /// <summary>
    /// Runtime constants recovered from the original NITE3W.EXE V1.10 and
    /// original game data. Keep this file limited to values backed by direct
    /// executable/data/save evidence in marek177/Nitemare3d-reversed.
    /// Partially named fields stay explicitly marked rather than guessed.
    /// </summary>
    public static class RecoveredRuntime
    {
        // MAP archive / world geometry.
        public const int MapWidth = 64;
        public const int MapHeight = 64;
        public const int MapCellBytes = 2;
        public const int MapLevelBytes = 8192;
        public const int MapHeaderBytes = 514;
        public const int Episode1Levels = 11; // E1M11 is the internal/demo map.
        public const int Episode2Levels = 10;
        public const int Episode3Levels = 10;
        public const int WorldUnitsPerTile = 64;

        // The supplied 11/10/10 payload counts are not treated as a proven
        // universal MAP-format ceiling. Historical MapEdit sources support a
        // larger editor level count; the original runtime ceiling is separate.

        // Player health/damage semantics recovered from the normal receiver.
        public const byte PlayerInitialHealth = 100;
        public const byte PlayerMaximumHealth = 100;
        public const byte PlayerAliveState = 0;
        public const byte PlayerNormalDeathState = 2;

        // Player movement/collision. The original uses a 54x54-world-unit AABB.
        public const int PlayerCollisionHalfExtentWorld = 27;
        public const float PlayerCollisionHalfExtentTiles =
            PlayerCollisionHalfExtentWorld / (float)WorldUnitsPerTile;
        public const float WorldUnitInTiles = 1.0f / WorldUnitsPerTile;
        public const int PlayerProximityThresholdWorld = 42; // Exact purpose PARTIAL.

        // Original input-mask anchors.
        public const ushort InputForward = 0x0002;
        public const ushort InputBackward = 0x0004;
        public const ushort InputTurnA = 0x0008; // Direction label still PARTIAL.
        public const ushort InputTurnB = 0x0010; // Direction label still PARTIAL.
        public const ushort InputFastModifier = 0x0020;
        public const ushort InputIncrementReset = 0x0040; // High-level label PARTIAL.
        public const ushort InputFire = 0x0080;
        public const ushort InputStrafeModifier = 0x0100;
        public const ushort InputUse = 0x0200;

        // Selected original gameplay globals / DS offsets.
        public const ushort DifficultyGlobal = 0x4C14;
        public const ushort ScoreLowGlobal = 0x4C16;
        public const ushort PlayerHealthGlobal = 0x4C1D;
        public const ushort SilverAmmoGlobal = 0x4C1F;
        public const ushort LaserAmmoGlobal = 0x4C20;
        public const ushort ActiveWeaponGlobal = 0x4C23;
        public const ushort WeaponJamGlobal = 0x4C2E;
        public const ushort WandAmmoGlobal = 0x4C44;
        public const ushort OmnipotentGlobal = 0x4BE5;
        public const ushort PlayerWorldXGlobal = 0x4BF6;
        public const ushort PlayerWorldYGlobal = 0x4BF8;

        // Difficulty ordering is supported by player->guard damage,
        // guard->player damage, and guard timing.
        public const ushort DifficultyEasier = 0;
        public const ushort DifficultyBaseline = 1;
        public const ushort DifficultyHarder = 2;

        // Runtime property flags recovered from NITE3W.EXE.
        public const ushort WallPropertyTable = 0x7E94;
        public const ushort ObjectPropertyTable = 0x7F94;
        public const byte WallHardBlock = 0x04;
        public const byte WallDynamicDoor = 0x08;
        public const byte WallScriptTouch = 0x40;
        public const byte ObjectRuntimePresent = 0x01;
        public const byte ObjectBlocksMovement = 0x02;
        public const byte ObjectSpecialTouch = 0x04;
        public const byte ObjectCreatesGuard = 0x08;

        // Original runtime limits / record sizes / selected bases.
        public const int MaxDoors = 64;
        public const int DoorRuntimeStride = 22;
        public const ushort DoorRuntimeBase = 0x9DD6;

        public const int MaxPanels = 32;
        public const int PanelRuntimeStride = 22;
        public const ushort PanelRuntimeBase = 0xA356;

        public const int MaxPushables = 12;
        public const int PushRuntimeStride = 6;
        public const ushort PushRuntimeBase = 0xA616;

        public const int MaxObjects = 350;
        public const int ObjectRuntimeStride = 28;
        public const ushort ObjectRuntimeBase = 0x6D66;
        public const ushort ObjectCountGlobal = 0x7E58;

        public const int MaxGuards = 100;
        public const int GuardRuntimeStride = 26;
        public const ushort GuardRuntimeBase = 0x93AE;
        public const ushort GuardCountGlobal = 0x7E5E;

        public const int MaxVectors = 1000;
        public const int VectorRuntimeStride = 28;
        public const ushort VectorCountGlobal = 0x7E56;
        public const int OrientationListCapacity = 333;

        public const int MaxSegments = 50;
        public const int WallSpanRuntimeStride = 20;
        public const int MaxImages = 70;

        public const int MaxProjectedSprites = 100;
        public const int ProjectedSpriteStride = 18;

        // Correction retained deliberately: earlier arithmetic guesses of
        // OBJECT=80 B and GUARD=98 B were wrong. Direct EXE indexing and
        // USER.SAV blocks prove 28 B and 26 B respectively.

        // Original renderer / viewport anchors.
        public const int FramebufferWidth = 320;
        public const int FramebufferHeight = 200;
        public const int FramebufferBytes = 64000;

        public const int ViewportXMin = 8;
        public const int ViewportXMax = 311;
        public const int ViewportYMin = 4;
        public const int ViewportYMax = 155;
        public const int ViewportWidth = 304;
        public const int ViewportHeight = 152;
        public const int ViewportCenterX = 160;
        public const int ViewportCenterY = 80;
        public const int ViewportCenterYQ4 = 1280;

        public const byte DefaultFloorPaletteIndex = 0x0C;
        public const byte DefaultCeilingPaletteIndex = 0x11;
        public const byte SpriteTransparentPaletteIndex = 0x29;

        // Per-column renderer tables in the original data segment.
        public const ushort ColumnOwnerBase = 0x53FE;
        public const int ColumnOwnerEntries = 320;
        public const int ColumnOwnerEntryBytes = 4; // Win16 far pointer.

        public const ushort WallOcclusionBase = 0x58FE;
        public const int WallOcclusionEntries = 320;
        public const int WallOcclusionEntryBytes = 2;

        public const ushort WallSpanCountGlobal = 0x5E7E;
        public const ushort WallSpanBase = 0x5E88;
        public const ushort ProjectedSpriteQueueBase = 0x6270;

        // Four orientation-specific pointer lists. Each list holds up to 333
        // far pointers to 28-byte VEC records; the real VEC pool holds 1000.
        public static readonly ushort[] VectorListCountGlobals =
        {
            0x697A, 0x697C, 0x697E, 0x6980
        };

        public static readonly ushort[] VectorListBases =
        {
            0x6982, 0x6EB6, 0x73EA, 0x791E
        };

        // Recovered VEC record offsets. Several animation/resource labels are
        // intentionally still PARTIAL in the source-of-truth repository.
        public const int VecWallIdOffset = 0x00;
        public const int VecTextureOffsetOffset = 0x01;
        public const int VecAnimAuxOffset = 0x02;
        public const int VecAnimFrameOffset = 0x03;
        public const int VecTextureSetOffset = 0x04;
        public const int VecFlagsOffset = 0x05;
        public const int VecRenderClassOffset = 0x06;
        public const int VecOrientationOffset = 0x07;
        public const int VecTimerOffset = 0x08;
        public const int VecX1Offset = 0x0C;
        public const int VecY1Offset = 0x0E;
        public const int VecX2Offset = 0x10;
        public const int VecY2Offset = 0x12;
        public const int VecScreenX1Offset = 0x14;
        public const int VecProjectedY1Q4Offset = 0x16;
        public const int VecScreenX2Offset = 0x18;
        public const int VecProjectedY2Q4Offset = 0x1A;

        public const byte VecFlagActive = 0x01;
        public const byte VecFlagSpecial04 = 0x04;
        public const byte VecFlagSpecial08 = 0x08;
        public const byte VecFlagMaskedLike = 0x10;
        public const byte VecFlagTextureUFlip = 0x20;

        // MAP boundary orientation used by the recovered VEC constructor.
        public const byte VecTop = 0;
        public const byte VecBottom = 1;
        public const byte VecRight = 2;
        public const byte VecLeft = 3;

        // Pushable movement: eight 8-unit updates == one 64-unit tile.
        public const byte PushableRuntimeClass = 0x28;
        public const int PushSteps = 8;
        public const int PushUnitsPerStep = 8;

        // Fresh GUARD strength is initialized to 0xFF by the original game.
        public const byte GuardInitialStrength = 0xFF;
        public const byte GuardPainState = 0x15;
        public const int GuardStateCount = 0x16; // States 0x00..0x15.

        // Original score switch for GUARD1..25. GUARD26/Dancers lies outside
        // this switch and follows the default zero-score path.
        public static readonly int[] GuardScorePoints =
        {
            25,    // GUARD1  Bat
            75,    // GUARD2  Frankenstein
            50,    // GUARD3  Mummy
            100,   // GUARD4  Skeleton
            250,   // GUARD5  Mrs H.
            150,   // GUARD6  Zelda
            200,   // GUARD7  Vampira
            100,   // GUARD8  Baddie #1
            100,   // GUARD9  Baddie #2
            0,     // GUARD10 Dracula
            150,   // GUARD11 Cemetery Gargoyle
            150,   // GUARD12 Garden Gargoyle
            200,   // GUARD13 unknown/unused identity
            -1000, // GUARD14 Penelope
            1000,  // GUARD15 Dr. Hamerstein
            100,   // GUARD16 Tall slim robot
            200,   // GUARD17 Trashcan robot
            0,     // GUARD18 Cannon
            25,    // GUARD19 Ghost
            100,   // GUARD20 Goldie
            100,   // GUARD21 Greenie
            250,   // GUARD22 Demon
            250,   // GUARD23 Alien #1
            200,   // GUARD24 Alien #2
            50,    // GUARD25 unknown/unused identity
            0      // GUARD26 Dancers/default path
        };

        // Ammo behavior confirmed in the executable.
        public const int AmmoPickupIncrement = 20;
        public const int AmmoForcedValue = 50;
        public const int AmmoThreshold = 100;

        // Known level/script events.
        public const byte EventEnteredTile = 0x16;
        public const byte EventSetWeaponJam = 0x47;
        public const byte EventClearWeaponJam = 0x48;
    }
}
