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

    public struct OriginalGuardAttackDamageResult
    {
        public int DistanceSeed;
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

        public static bool GuardAttackUsesRandom(byte objectClass)
        {
            switch (objectClass)
            {
                case 0x08:
                case 0x09:
                case 0x0A:
                case 0x11:
                case 0x12:
                case 0x13:
                case 0x14:
                    return true;
                default:
                    return false;
            }
        }

        /// <summary>
        /// Exact 16-bit distance helper used by FUN_1010_A1EA through
        /// FUN_1018_32AA/FUN_1018_324A. World coordinates are converted to
        /// 64-unit tiles before the original integer square-root rounding rule.
        /// </summary>
        public static int ComputeGuardAttackDistanceMetric(
            short guardWorldX,
            short guardWorldY,
            short playerWorldX,
            short playerWorldY)
        {
            int dx = (guardWorldX >> 6) - (playerWorldX >> 6);
            int dy = (guardWorldY >> 6) - (playerWorldY >> 6);
            return OriginalRoundedSqrt(dx * dx + dy * dy);
        }

        public static int OriginalRoundedSqrt(int squaredDistance)
        {
            if (squaredDistance <= 1)
                return squaredDistance < 0 ? 0 : squaredDistance;

            int root = (int)System.Math.Sqrt(squaredDistance);
            int remainder = squaredDistance - root * root;

            // FUN_1018_324A compares the remainder against root-1.
            if (remainder >= root - 1)
                root++;

            return root;
        }

        public static OriginalGuardAttackDamageResult ComputeGuardToPlayerFromWorld(
            short guardWorldX,
            short guardWorldY,
            short playerWorldX,
            short playerWorldY,
            byte objectClass,
            byte difficulty,
            bool class16FullDamageGate,
            ushort rngValue)
        {
            return ComputeGuardToPlayer(
                ComputeGuardAttackDistanceMetric(
                    guardWorldX, guardWorldY, playerWorldX, playerWorldY),
                objectClass,
                difficulty,
                class16FullDamageGate,
                rngValue);
        }

        /// <summary>
        /// Confirmed GUARD-to-player damage producer recovered from NITE3W 1.10.
        /// distanceMetric is the recovered result of FUN_1018_32AA. Call
        /// ComputeGuardToPlayerFromWorld when world coordinates are available.
        /// The class-0x16 boolean mirrors the original
        /// (DAT_1048_7e52 == 3 || DAT_1048_51a6 != 0) gate.
        /// </summary>
        public static OriginalGuardAttackDamageResult ComputeGuardToPlayer(
            int distanceMetric,
            byte objectClass,
            byte difficulty,
            bool class16FullDamageGate,
            ushort rngValue)
        {
            int seed = distanceMetric > 0 ? 100 / distanceMetric : 100;
            int damage = ApplyGuardAttackClassTransform(
                seed, objectClass, class16FullDamageGate, rngValue);

            int classTransformed = damage;

            // Enemy-to-player difficulty scaling is the inverse of the
            // player-to-GUARD path: hard doubles, easy halves.
            if (difficulty == 2)
                damage *= 2;
            else if (difficulty == 0)
                damage /= 2;

            return new OriginalGuardAttackDamageResult
            {
                DistanceSeed = seed,
                ClassTransformed = classTransformed,
                DifficultyTransformed = damage,
                StoredByte = unchecked((byte)damage)
            };
        }

        /// <summary>
        /// Exact class switch from FUN_1010_A1EA after the 100/distance seed.
        /// rngValue is consumed only by classes whose original branch sampled RNG.
        /// </summary>
        public static int ApplyGuardAttackClassTransform(
            int distanceSeed,
            byte objectClass,
            bool class16FullDamageGate,
            ushort rngValue)
        {
            switch (objectClass)
            {
                case 0x08:
                    return rngValue & 0x07;

                case 0x09:
                case 0x0A:
                    return rngValue & 0x0F;

                case 0x0B:
                    return distanceSeed / 4;

                case 0x0C:
                case 0x1D:
                case 0x1E:
                    return distanceSeed;

                case 0x11:
                case 0x12:
                case 0x13:
                case 0x14:
                    return rngValue & 0x1F;

                case 0x16:
                    return class16FullDamageGate ? 100 : 0x21;

                case 0x19:
                    return 100;

                // 0x0D-0x10, 0x15, 0x17-0x18, 0x1A-0x1C and
                // the original default branch (including 0x1F) halve the seed.
                default:
                    return distanceSeed / 2;
            }
        }

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
