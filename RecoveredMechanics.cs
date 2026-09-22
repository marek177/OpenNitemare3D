namespace Nitemare3D
{
    public static class RecoveredMechanics
    {
        public const byte WallHardBlock = 0x04;
        public const byte WallDynamicDoor = 0x08;
        public const byte WallScriptTouch = 0x40;
        public const byte ObjectRuntimePresent = 0x01;
        public const byte ObjectBlocksMovement = 0x02;
        public const byte ObjectSpecialTouch = 0x04;
        public const byte ObjectCreatesGuard = 0x08;

        public const int ObjectFlagsOffset = 0x05;
        public const int ObjectClassOffset = 0x06;
        public const int ObjectGuardIndexOffset = 0x07;
        public const int ObjectMapBindingOffset = 0x0C;
        public const int ObjectWorldXOffset = 0x10;
        public const int ObjectWorldYOffset = 0x12;
        public const int ObjectProjectedDamageBaselineOffset = 0x18;

        public const int GuardTimerOffset = 0x06;
        public const int GuardObjectSlotOffset = 0x08;
        public const int GuardStrategyOffset = 0x0A;
        public const int GuardStateOffset = 0x0B;
        public const int GuardNextStateOffset = 0x0C;
        public const int GuardStrengthOffset = 0x10;

        public static int ClampHealth(int health)
        {
            if (health < 0) return 0;
            return health > OriginalRuntime.PlayerMaxHealth ? OriginalRuntime.PlayerMaxHealth : health;
        }

        public static int ClampNormalAmmo(int ammo)
        {
            if (ammo < 0) return 0;
            return ammo > OriginalRuntime.NormalAmmoCap ? OriginalRuntime.NormalAmmoCap : ammo;
        }

        public static bool HasInput(ushort mask, ushort bit) => (mask & bit) != 0;
    }
}
