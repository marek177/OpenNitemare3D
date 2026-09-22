namespace Nitemare3D
{
    // Win16 reference facts. This class has no side effects on the running game.
    // See docs/ORIGINAL_RUNTIME_DELTA_2026-09-22.md.
    public static class RecoveredInteractionFacts
    {
        public const byte ControlWallClass = 0x03;
        public const byte ControlObjectClass = 0x03;
        public const byte RemoteDoorVerticalClass = 0x3B;
        public const byte RemoteDoorHorizontalClass = 0x3C;
        public const byte RemoteOpenCommand = 0x1E;
        public const byte RemoteCloseCommand = 0x1F;
        public const byte MovementScriptWallFlag = 0x40;
        public const byte PortalPentagramMask = 0x0F;
        public const byte PortalEntryClass = 0x15;
        public const byte PortalExitClass = 0x16;
        public const byte ExplodingWallRuntimeClass = 0x2D;
        public const byte ExplodingWallSound = 0x29;

        public static bool IsRemoteDoorClass(byte value) =>
            value == RemoteDoorVerticalClass || value == RemoteDoorHorizontalClass;

        public static bool CanOpenRemoteDoor(byte state) => state == 1 || state == 3;
        public static bool CanCloseRemoteDoor(byte state) => state == 0 || state == 2;

        public static bool HasCard(ushort cards, byte group) =>
            group < 16 && ((cards >> group) & 1) != 0;

        public static bool HasAllPentagrams(byte mask) =>
            (mask & PortalPentagramMask) == PortalPentagramMask;
    }
}
