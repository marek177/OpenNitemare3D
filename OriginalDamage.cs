namespace Nitemare3D
{
    public enum OriginalWeaponSelector : byte
    {
        SingleShotLaser = 0,
        MagicWand = 1,
        SilverPistol = 2,
        ContinuousLaser = 3
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
    /// The caller must supply the projected OBJECT baseline and RNG value from
    /// the same frame/path; this is intentionally not a fixed weapon damage table.
    /// </summary>
    public static class OriginalDamage
    {
        public static OriginalDamageResult ComputePlayerToGuard(
            short projectedBaseRow,
            short viewportCenterY,
            byte objectClass,
            byte weaponSelector,
            byte difficulty,
            byte episode,
            ushort rngValue)
        {
            int raw = 8 * (projectedBaseRow - viewportCenterY) + (rngValue % 25);
            int damage = raw;

            switch (objectClass)
            {
                case 0x0C:
                case 0x1D:
                    damage >>= 3;
                    break;

                case 0x0D:
                    damage = (weaponSelector == 1 || weaponSelector == 2)
                        ? damage >> 1
                        : damage >> 3;
                    break;

                case 0x0E:
                case 0x11:
                case 0x14:
                    damage = weaponSelector == 2
                        ? damage >> 1
                        : damage >> 3;
                    break;

                case 0x0F:
                case 0x10:
                    damage = weaponSelector == 1 ? 0 : damage >> 8;
                    break;

                case 0x12:
                case 0x13:
                    damage = weaponSelector == 1 ? 0 : damage >> 2;
                    break;

                case 0x15:
                    damage = 0;
                    break;

                case 0x16:
                    damage = episode == 3 ? 3 : 0;
                    break;

                case 0x17:
                    damage = weaponSelector == 1 ? damage >> 8 : damage >> 2;
                    break;

                case 0x18:
                    if (weaponSelector == 1) damage >>= 8;
                    else if (weaponSelector == 2) damage >>= 4;
                    else damage >>= 3;
                    break;

                case 0x19:
                    damage = 0;
                    break;

                case 0x1A:
                    damage = weaponSelector == 1 ? damage >> 1 : 0;
                    break;

                case 0x1B:
                case 0x1C:
                    damage >>= 1;
                    break;

                case 0x1E:
                    damage = weaponSelector == 1 ? 0 : damage >> 3;
                    break;

                case 0x1F:
                    damage = weaponSelector == 1 ? 0 : damage >> 2;
                    break;
            }

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
    }
}
