namespace Nitemare3D
{
    public enum OriginalWeaponSelector : byte
    {
        SingleShotLaser = 0,
        MagicWand = 1,
        SilverPistol = 2,
        ContinuousLaser = 3,
        None = 0xFF
    }

    public struct OriginalDamageResult
    {
        public int RawSeed;
        public int ClassTransformed;
        public int DifficultyTransformed;
        public byte StoredByte;
    }

    /// <summary>
    /// Confirmed player-to-GUARD damage producer recovered from NITE3W 1.10.
    /// The caller supplies the projected OBJECT baseline, RNG value and current
    /// Hamerstein gate. Damage is not a fixed per-weapon integer.
    /// </summary>
    public static class OriginalDamage
    {
        public const byte HamersteinGateRequiredValue = 3;
        public const byte HamersteinBaseDamage = 3;

        public static OriginalDamageResult ComputePlayerToGuard(
            short projectedBaseRow,
            short viewportCenterY,
            byte objectClass,
            byte weaponSelector,
            byte difficulty,
            byte hamersteinGateValue,
            ushort rngValue)
        {
            int raw = 8 * (projectedBaseRow - viewportCenterY) + (rngValue % 25);
            int damage = ApplyClassWeaponTransform(
                raw, objectClass, weaponSelector, hamersteinGateValue);

            int classTransformed = damage;

            if (difficulty == 2)
                damage /= 2;
            else if (difficulty == 0)
                damage *= 2;

            if (damage > 255)
                damage = 255;

            return new OriginalDamageResult
            {
                RawSeed = raw,
                ClassTransformed = classTransformed,
                DifficultyTransformed = damage,
                StoredByte = unchecked((byte)damage)
            };
        }

        public static int ApplyClassWeaponTransform(
            int rawDamage,
            byte objectClass,
            byte weaponSelector,
            byte hamersteinGateValue = 0)
        {
            if (rawDamage <= 0)
                return rawDamage;

            bool wand = weaponSelector == (byte)OriginalWeaponSelector.MagicWand;
            bool silver = weaponSelector == (byte)OriginalWeaponSelector.SilverPistol;

            switch (objectClass)
            {
                case 0x0C:
                case 0x1D:
                    return rawDamage / 8;

                case 0x0D:
                    return wand ? rawDamage / 2 : rawDamage / 8;

                case 0x0E:
                case 0x11:
                case 0x14:
                    return (wand || silver) ? rawDamage / 2 : rawDamage / 8;

                case 0x0F:
                case 0x10:
                    return wand ? rawDamage / 2 : rawDamage / 256;

                case 0x12:
                case 0x13:
                    return rawDamage / 4;

                case 0x15:
                case 0x19:
                    return 0;

                case 0x16:
                    return hamersteinGateValue == HamersteinGateRequiredValue
                        ? HamersteinBaseDamage
                        : 0;

                case 0x17:
                    return wand ? rawDamage / 256 : rawDamage / 4;

                case 0x18:
                    if (wand) return rawDamage / 256;
                    if (silver) return rawDamage / 16;
                    return rawDamage / 8;

                case 0x1A:
                    return wand ? rawDamage / 2 : 0;

                case 0x1B:
                case 0x1C:
                    return rawDamage / 2;

                case 0x1E:
                    return wand ? 0 : rawDamage / 8;

                case 0x1F:
                    return wand ? 0 : rawDamage / 4;

                default:
                    return rawDamage;
            }
        }
    }
}
