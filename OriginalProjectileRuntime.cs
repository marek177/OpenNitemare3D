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

        public static bool TryAllocateAndInitialize(
            OriginalProjectileRecord[] pool,
            byte weaponSelector,
            short worldX,
            short worldY,
            byte sequenceBase,
            out int slotIndex)
        {
            slotIndex = FirstFreeSlot(pool);
            if (slotIndex < 0)
                return false;

            if (!InitializeSpawn(
                    ref pool[slotIndex],
                    weaponSelector,
                    worldX,
                    worldY,
                    sequenceBase))
            {
                slotIndex = -1;
                return false;
            }

            return true;
        }

        public static bool InitializeSpawn(
            ref OriginalProjectileRecord projectile,
            byte weaponSelector,
            short worldX,
            short worldY,
            byte sequenceBase)
        {
            if (!TryGetSequenceOffsets(weaponSelector, out var offsets))
                return false;

            projectile = default;
            projectile.State = (byte)OriginalProjectileState.Flying;
            projectile.RenderObject.Component03 = 0;
            projectile.RenderObject.DefinitionId =
                (byte)(sequenceBase + offsets.Flight);
            projectile.RenderObject.Flags = 0x01;
            projectile.RenderObject.ObjectClass = 5;
            projectile.RenderObject.WorldX = worldX;
            projectile.RenderObject.WorldY = worldY;
            projectile.RenderObject.Runtime1A = 5;
            return true;
        }

        public static bool EnterImpact(
            ref OriginalProjectileRecord projectile,
            byte weaponSelector,
            byte sequenceBase)
        {
            if (!TryGetSequenceOffsets(weaponSelector, out var offsets))
                return false;

            projectile.State = (byte)OriginalProjectileState.Impact;
            projectile.RenderObject.Component03 = 0;
            projectile.RenderObject.DefinitionId =
                (byte)(sequenceBase + offsets.Impact);
            projectile.RenderObject.Flags =
                (byte)(projectile.RenderObject.Flags | 0x10);
            return true;
        }

        /// <summary>
        /// Advances one animation frame after the caller has determined that the
        /// sequence deadline elapsed. Flight loops; impact frees the slot after
        /// the final frame.
        /// </summary>
        public static void AdvanceAnimationFrame(
            ref OriginalProjectileRecord projectile,
            int frameCount)
        {
            if (frameCount <= 0 ||
                projectile.State == (byte)OriginalProjectileState.Free)
            {
                return;
            }

            int nextFrame = projectile.RenderObject.Component03 + 1;

            if (projectile.State == (byte)OriginalProjectileState.Flying)
            {
                if (nextFrame >= frameCount)
                    nextFrame = 0;

                projectile.RenderObject.Component03 = (sbyte)nextFrame;
                return;
            }

            if (projectile.State == (byte)OriginalProjectileState.Impact)
            {
                if (nextFrame >= frameCount)
                {
                    projectile.State = (byte)OriginalProjectileState.Free;
                    return;
                }

                projectile.RenderObject.Component03 = (sbyte)nextFrame;
            }
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
