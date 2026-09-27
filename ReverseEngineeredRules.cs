namespace Nitemare3D
{
    // Runtime constants recovered from the original NITE3W.EXE.
    // Keep these separate from presentation/asset IDs: they describe logical
    // wall/object classes used by the original interaction dispatcher.
    public static class ReverseEngineeredRules
    {
        public const ushort UseInputMask = 0x0200;

        public const int InternalUnitsPerTile = 64;
        public const int PlayerCollisionHalfExtent = 27;

        public const int DoorRuntimeStride = 0x16;
        public const int MaxRuntimeDoors = 64;
        public const int PanelRuntimeStride = 0x16;
        public const int MaxRuntimePanels = 32;
        public const int PushRuntimeStride = 6;
        public const int MaxRuntimePushables = 12;

        public const byte WallPropertyDynamicDoor = 0x08;
        public const byte BlockingProperty = 0x02;

        public const byte RemoteTerminalWallType = 0x03;
        public const byte ClimbWallTypeFirst = 0x0D;
        public const byte ClimbWallTypeLast = 0x14;
        public const byte OtherSideWallTypeFirst = 0x15;
        public const byte OtherSideWallTypeLast = 0x18;
        public const byte ColoredKeyGateFirst = 0x19;
        public const byte ColoredKeyGateLast = 0x1C;
        public const byte FloorSelectorFirst = 0x1D;
        public const byte FloorSelectorLast = 0x24;
        public const byte GoDownFirst = 0x25;
        public const byte GoDownLast = 0x2C;

        public const byte CombinationObjectType = 0x26;
        public const byte PushableObjectType = 0x28;
        public const byte PanelObjectType = 0x03;

        // Octants 0..7 are quantized to one adjacent cardinal cell by USE.
        public static readonly int[] UseCellDelta =
        {
            -64, +1, +1, +64, +64, -1, -1, -64
        };

        // Original push update deltas: eight 8-unit updates == one 64-unit tile.
        public static readonly int[] PushDeltaX =
        {
             0, +8, +8,  0,  0, -8, -8,  0
        };

        public static readonly int[] PushDeltaY =
        {
            -8,  0,  0, +8, +8,  0,  0, -8
        };

        public static string ColoredKeyName(int index)
        {
            switch (index)
            {
                case 0: return "Red key";
                case 1: return "Green key";
                case 2: return "Blue key";
                case 3: return "Yellow key";
                default: return "Unknown key";
            }
        }

        public static string IdCardName(int index)
        {
            switch (index)
            {
                case 0: return "Red ID card";
                case 1: return "Yellow ID card";
                default: return "Unknown ID card";
            }
        }

        public static bool IsReusableColoredKeyGate(byte logicalWallType)
        {
            return logicalWallType >= ColoredKeyGateFirst &&
                   logicalWallType <= ColoredKeyGateLast;
        }

        public static int RequiredColoredKeyIndex(byte logicalWallType)
        {
            return IsReusableColoredKeyGate(logicalWallType)
                ? logicalWallType - ColoredKeyGateFirst
                : -1;
        }

        public static bool IsClimbSelector(byte logicalWallType)
        {
            return logicalWallType >= ClimbWallTypeFirst &&
                   logicalWallType <= ClimbWallTypeLast;
        }

        public static bool IsFloorSelector(byte logicalWallType)
        {
            return logicalWallType >= FloorSelectorFirst &&
                   logicalWallType <= FloorSelectorLast;
        }

        public static bool IsGoDownConfirmation(byte logicalWallType)
        {
            return logicalWallType >= GoDownFirst &&
                   logicalWallType <= GoDownLast;
        }
    }
}
