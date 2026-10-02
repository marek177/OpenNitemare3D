using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Nitemare3D
{
    /// <summary>
    /// Production-side storage for the recovered fixed Win16 OBJECT/GUARD pools.
    /// The historical Entity classes still drive gameplay; this pool is the bridge
    /// used while behavior is migrated state-by-state to the original runtime model.
    /// </summary>
    public static class OriginalRuntimeState
    {
        sealed class Binding
        {
            public int ObjectSlot = -1;
            public int GuardSlot = -1;
        }

        public static readonly OriginalObjectRecord[] Objects =
            new OriginalObjectRecord[OriginalRuntime.MaxObjects];

        public static readonly OriginalGuardRecord[] Guards =
            new OriginalGuardRecord[OriginalRuntime.MaxGuards];

        public static readonly OriginalGuardWakeCache GuardWakeCache =
            new OriginalGuardWakeCache();

        static readonly Dictionary<Entity, Binding> bindings =
            new Dictionary<Entity, Binding>();

        public static int ObjectCount { get; private set; }
        public static int GuardCount { get; private set; }

        public static bool LayoutMatchesRecoveredRuntime =>
            Marshal.SizeOf<OriginalObjectRecord>() == OriginalRuntime.ObjectRuntimeStride &&
            Marshal.SizeOf<OriginalGuardRecord>() == OriginalRuntime.GuardRuntimeStride;

        public static void Reset()
        {
            Array.Clear(Objects, 0, Objects.Length);
            Array.Clear(Guards, 0, Guards.Length);
            GuardWakeCache.Clear();
            bindings.Clear();
            ObjectCount = 0;
            GuardCount = 0;
        }

        public static bool TryMapPortGuardClass(GuardType type, out byte objectClass)
        {
            switch (type)
            {
                case GuardType.Bat:
                    objectClass = 0x08;
                    return true;
                case GuardType.Frankenstein:
                    objectClass = 0x09;
                    return true;
                case GuardType.Mummy:
                    objectClass = 0x0A;
                    return true;
                case GuardType.Skeleton:
                    objectClass = 0x0B;
                    return true;
                case GuardType.HumanBlue:
                    objectClass = 0x0F;
                    return true;
                case GuardType.HumanGreen:
                    objectClass = 0x10;
                    return true;
                case GuardType.Penelope:
                    objectClass = OriginalRuntime.PenelopeClass;
                    return true;
                case GuardType.DRHammerstein:
                    objectClass = OriginalRuntime.DrHamersteinClass;
                    return true;
                default:
                    // Current port names such as Witch/Gargorle/Robot are not
                    // specific enough to bind to one recovered original class.
                    objectClass = 0;
                    return false;
            }
        }

        public static bool RegisterGuard(Entity entity, GuardType type)
        {
            if (!TryMapPortGuardClass(type, out byte objectClass))
            {
                return false;
            }

            if (ObjectCount >= Objects.Length)
                throw new InvalidOperationException("Original OBJECT pool capacity exceeded.");
            if (GuardCount >= Guards.Length)
                throw new InvalidOperationException("Original GUARD pool capacity exceeded.");

            int objectSlot = ObjectCount++;
            int guardSlot = GuardCount++;

            ref var obj = ref Objects[objectSlot];
            obj.Flags = (byte)(RecoveredMechanics.ObjectRuntimePresent |
                                RecoveredMechanics.ObjectCreatesGuard);
            obj.ObjectClass = objectClass;
            obj.GuardIndex = (byte)guardSlot;
            WriteWorldPosition(ref obj, entity.position);

            ref var guard = ref Guards[guardSlot];
            guard.ObjectSlot = (ushort)objectSlot;
            guard.Strength = OriginalRuntime.GuardInitialStrength;

            bindings[entity] = new Binding
            {
                ObjectSlot = objectSlot,
                GuardSlot = guardSlot
            };

            return true;
        }

        public static void SyncGuardPosition(Entity entity)
        {
            if (!bindings.TryGetValue(entity, out var binding) ||
                binding.ObjectSlot < 0)
            {
                return;
            }

            WriteWorldPosition(ref Objects[binding.ObjectSlot], entity.position);
        }

        public static int WakeGuardsAfterPlayerFire(
            byte selector,
            Func<int, ushort> randomForGuard)
        {
            return GuardWakeCache.Wake(
                selector,
                Guards,
                GuardCount,
                randomForGuard);
        }

        public static OriginalGuardDispatchResult TickConfirmedAutonomousState(Entity entity)
        {
            if (!bindings.TryGetValue(entity, out var binding) ||
                binding.GuardSlot < 0 ||
                binding.ObjectSlot < 0)
            {
                return OriginalGuardDispatchResult.NotHandled;
            }

            ref var guard = ref Guards[binding.GuardSlot];
            ref var obj = ref Objects[binding.ObjectSlot];

            switch ((OriginalGuardState)guard.State)
            {
                case OriginalGuardState.Delay:
                    return OriginalGuardDispatcher.TickDelay(ref guard);

                case OriginalGuardState.Transition13:
                {
                    var result = OriginalGuardDispatcher.TickState13(
                        ref guard,
                        ref obj,
                        (worldX, worldY) =>
                        {
                            int tileX = worldX >> 6;
                            int tileY = worldY >> 6;
                            return Level.IsWalkable(tileX, tileY, entity);
                        });

                    if (result == OriginalGuardDispatchResult.Moved)
                    {
                        entity.position.X =
                            (float)obj.WorldX / OriginalRuntime.WorldUnitsPerTile;
                        entity.position.Y =
                            (float)obj.WorldY / OriginalRuntime.WorldUnitsPerTile;
                    }

                    return result;
                }

                default:
                    return OriginalGuardDispatchResult.NotHandled;
            }
        }

        public static bool EnterStrategy3TimedMove(Entity entity, ushort randomValue)
        {
            if (!bindings.TryGetValue(entity, out var binding) ||
                binding.GuardSlot < 0)
            {
                return false;
            }

            ref var guard = ref Guards[binding.GuardSlot];
            OriginalGuardDispatcher.EnterStrategy3TimedMove(ref guard, randomValue);
            return true;
        }

        public static bool CompleteDeferredGuardState(Entity entity)
        {
            if (!bindings.TryGetValue(entity, out var binding) ||
                binding.GuardSlot < 0)
            {
                return false;
            }

            ref var guard = ref Guards[binding.GuardSlot];
            return OriginalGuardDispatcher.CompleteDeferredState(ref guard) ==
                   OriginalGuardDispatchResult.Completed;
        }

        public static bool CompleteGuardState06(Entity entity)
        {
            if (!bindings.TryGetValue(entity, out var binding) ||
                binding.GuardSlot < 0)
            {
                return false;
            }

            ref var guard = ref Guards[binding.GuardSlot];
            return OriginalGuardDispatcher.CompleteState06(ref guard) ==
                   OriginalGuardDispatchResult.Completed;
        }

        public static bool CompleteGuardState11(Entity entity)
        {
            if (!bindings.TryGetValue(entity, out var binding) ||
                binding.GuardSlot < 0)
            {
                return false;
            }

            ref var guard = ref Guards[binding.GuardSlot];
            return OriginalGuardDispatcher.CompleteState11(ref guard) ==
                   OriginalGuardDispatchResult.Completed;
        }

        public static bool CompletePainReaction(Entity entity)
        {
            if (!bindings.TryGetValue(entity, out var binding) ||
                binding.GuardSlot < 0)
            {
                return false;
            }

            ref var guard = ref Guards[binding.GuardSlot];
            return OriginalGuardDispatcher.CompletePainReaction(ref guard) ==
                   OriginalGuardDispatchResult.Completed;
        }

        public static bool TryGetGuardRecord(Entity entity, out OriginalGuardRecord guard)
        {
            if (bindings.TryGetValue(entity, out var binding) &&
                binding.GuardSlot >= 0)
            {
                guard = Guards[binding.GuardSlot];
                return true;
            }

            guard = default;
            return false;
        }

        public static bool TryGetObjectRecord(Entity entity, out OriginalObjectRecord obj)
        {
            if (bindings.TryGetValue(entity, out var binding) &&
                binding.ObjectSlot >= 0)
            {
                obj = Objects[binding.ObjectSlot];
                return true;
            }

            obj = default;
            return false;
        }

        public enum GuardHitResult
        {
            NoRuntimeBinding,
            NoDamage,
            PainReaction,
            Killed,
            DraculaTransformed
        }

        /// <summary>
        /// Applies the confirmed normal GUARD damage receiver path.
        /// Special strategy-specific hit branches are deliberately not folded into
        /// this helper until their semantics are fully closed.
        /// </summary>
        public static GuardHitResult ApplyNormalGuardDamage(Entity entity, byte damage)
        {
            if (!bindings.TryGetValue(entity, out var binding) ||
                binding.GuardSlot < 0 ||
                binding.ObjectSlot < 0)
            {
                return GuardHitResult.NoRuntimeBinding;
            }

            if (damage == 0)
            {
                return GuardHitResult.NoDamage;
            }

            ref var guard = ref Guards[binding.GuardSlot];
            ref var obj = ref Objects[binding.ObjectSlot];

            if (damage >= guard.Strength)
            {
                if (obj.ObjectClass == OriginalRuntime.DraculaPhase1Class)
                {
                    obj.ObjectClass = OriginalRuntime.DraculaBatPhase2Class;
                    guard.Strength = OriginalRuntime.GuardInitialStrength;
                    guard.State = (byte)OriginalGuardState.Move08;
                    guard.NextState = (byte)OriginalGuardState.Active02;
                    guard.Timer = 1;
                    return GuardHitResult.DraculaTransformed;
                }

                guard.Strength = 0;
                return GuardHitResult.Killed;
            }

            guard.Strength = (byte)(guard.Strength - damage);
            guard.ResultOctant = 8;
            guard.NextState = guard.State;
            guard.State = (byte)OriginalGuardState.Pain15;
            return GuardHitResult.PainReaction;
        }

        public static void SetGuardStrength(Entity entity, byte strength)
        {
            if (bindings.TryGetValue(entity, out var binding) &&
                binding.GuardSlot >= 0)
            {
                Guards[binding.GuardSlot].Strength = strength;
            }
        }

        static void WriteWorldPosition(ref OriginalObjectRecord obj, Vec2 position)
        {
            obj.WorldX = ToWorldCoordinate(position.X);
            obj.WorldY = ToWorldCoordinate(position.Y);
        }

        static short ToWorldCoordinate(float tileCoordinate)
        {
            int world = (int)MathF.Round(
                tileCoordinate * OriginalRuntime.WorldUnitsPerTile);

            if (world < short.MinValue) return short.MinValue;
            if (world > short.MaxValue) return short.MaxValue;
            return (short)world;
        }
    }
}
