using System;
using System.Runtime.InteropServices;

namespace Nitemare3D
{
    public enum OriginalProjectileState : byte
    {
        Free = 0,
        Flying = 1,
        Impact = 2
    }

    [StructLayout(LayoutKind.Explicit, Pack = 1, Size = OriginalRuntime.ProjectileRuntimeStride)]
    public struct OriginalProjectileRecord
    {
        [FieldOffset(0x00)] public short XIsMajorAxis;
        [FieldOffset(0x02)] public short LineError;
        [FieldOffset(0x04)] public short MinorErrorStep;
        [FieldOffset(0x06)] public short MajorErrorFixup;
        [FieldOffset(0x08)] public short StepX;
        [FieldOffset(0x0A)] public short StepY;
        [FieldOffset(0x0C)] public byte State;
        [FieldOffset(0x0D)] public byte Unknown0D;
        [FieldOffset(0x0E)] public OriginalObjectRecord RenderObject;
    }

    public struct OriginalProjectileSequenceOffsets
    {
        public byte Flight;
        public byte Impact;

        public OriginalProjectileSequenceOffsets(byte flight, byte impact)
        {
            Flight = flight;
            Impact = impact;
        }
    }

    /// <summary>
    /// Executable-backed projectile layout and collision helpers.
    /// Exact updates-per-second remain calibration-dependent in the original.
    /// </summary>
    public static class OriginalProjectileRuntime
    {
        public const int GuardHitTolerance = 9;
        public const int RenderDistanceThreshold = 20;

        public static bool WeaponUsesProjectile(byte weaponSelector)
        {
            return weaponSelector == 0 || weaponSelector == 1 || weaponSelector == 3;
        }

        public static bool TryGetSequenceOffsets(
            byte weaponSelector,
            out OriginalProjectileSequenceOffsets offsets)
        {
            switch (weaponSelector)
            {
                case 0:
                case 3:
                    offsets = new OriginalProjectileSequenceOffsets(0, 1);
                    return true;
                case 1:
                    offsets = new OriginalProjectileSequenceOffsets(2, 3);
                    return true;
                default:
                    offsets = default;
                    return false;
            }
        }

        public static int FirstFreeSlot(OriginalProjectileRecord[] pool)
        {
            if (pool == null)
                return -1;

            int count = Math.Min(pool.Length, OriginalRuntime.MaxProjectiles);
            for (int i = 0; i < count; i++)
            {
                if (pool[i].State == (byte)OriginalProjectileState.Free)
                    return i;
            }

            return -1;
        }

        public static bool HitsGuard(
            int projectileX,
            int projectileY,
            int guardX,
            int guardY)
        {
            long dx = (long)projectileX - guardX;
            long dy = (long)projectileY - guardY;

            return dx >= -GuardHitTolerance && dx <= GuardHitTolerance &&
                   dy >= -GuardHitTolerance && dy <= GuardHitTolerance;
        }

        public static bool NeedsProjection(
            int projectileX,
            int projectileY,
            int playerX,
            int playerY)
        {
            long dx = (long)projectileX - playerX;
            long dy = (long)projectileY - playerY;

            return dx > RenderDistanceThreshold || dx < -RenderDistanceThreshold ||
                   dy > RenderDistanceThreshold || dy < -RenderDistanceThreshold;
        }

        public static void EnterImpact(ref OriginalProjectileRecord projectile)
        {
            projectile.State = (byte)OriginalProjectileState.Impact;
            projectile.RenderObject.Component03 = 0;
            projectile.RenderObject.Flags =
                (byte)(projectile.RenderObject.Flags | 0x10);
        }
    }

    public sealed class OriginalProjectilePool
    {
        public readonly OriginalProjectileRecord[] Slots =
            new OriginalProjectileRecord[OriginalRuntime.MaxProjectiles];

        public void Clear()
        {
            Array.Clear(Slots, 0, Slots.Length);
        }

        public int FirstFreeSlot()
        {
            return OriginalProjectileRuntime.FirstFreeSlot(Slots);
        }
    }
}
