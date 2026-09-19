namespace Nitemare3D
{
    /// <summary>
    /// Runtime constants recovered from the original NITE3W.EXE V1.10 and
    /// original game data. Keep this file limited to values backed by direct
    /// executable/data evidence in marek177/Nitemare3d-reversed.
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

        // Player movement/collision. The original uses a 54x54-unit AABB.
        public const int PlayerCollisionHalfExtentWorld = 27;
        public const float PlayerCollisionHalfExtentTiles =
            PlayerCollisionHalfExtentWorld / (float)WorldUnitsPerTile;
        public const float WorldUnitInTiles = 1.0f / WorldUnitsPerTile;

        // Original input-mask anchors.
        public const ushort InputFire = 0x0080;
        public const ushort InputStrafeModifier = 0x0100;
        public const ushort InputUse = 0x0200;

        // Runtime property flags recovered from NITE3W.EXE.
        public const byte WallHardBlock = 0x04;
        public const byte WallDynamicDoor = 0x08;
        public const byte WallScriptTouch = 0x40;
        public const byte ObjectRuntimePresent = 0x01;
        public const byte ObjectBlocksMovement = 0x02;
        public const byte ObjectSpecialTouch = 0x04;
        public const byte ObjectCreatesGuard = 0x08;

        // Original runtime limits / record sizes.
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
        public const int MaxSegments = 50;
        public const int MaxImages = 70;
        public const int OrientationListCapacity = 333;

        // Pushable movement: eight 8-unit updates == one 64-unit tile.
        public const byte PushableRuntimeClass = 0x28;
        public const int PushSteps = 8;
        public const int PushUnitsPerStep = 8;

        // Fresh GUARD strength is initialized to 0xFF by the original game.
        public const byte GuardInitialStrength = 0xFF;

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
