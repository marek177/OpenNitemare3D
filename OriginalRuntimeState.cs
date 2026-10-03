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

        public static readonly OriginalObjectDefinitionCatalog ObjectDefinitions =
            new OriginalObjectDefinitionCatalog();

        public static readonly OriginalProjectilePool ProjectilePool =
            new OriginalProjectilePool();

        static readonly Dictionary<Entity, Binding> bindings =
            new Dictionary<Entity, Binding>();

        public static int ObjectCount { get; private set; }
        public static int GuardCount { get; private set; }

        // DAT_1048_4C14. FUN_BA16 initializes this to 1 (normal); the
        // difficulty menu writes 0/1/2. It intentionally survives level Reset().
        public static byte Difficulty { get; private set; } =
            OriginalRuntime.DefaultDifficulty;

        public static void SetDifficulty(byte difficulty)
        {
            if (difficulty > OriginalRuntime.DifficultyHard)
                throw new ArgumentOutOfRangeException(nameof(difficulty));

            Difficulty = difficulty;
        }

        public static bool LayoutMatchesRecoveredRuntime =>
            Marshal.SizeOf<OriginalObjectRecord>() == OriginalRuntime.ObjectRuntimeStride &&
            Marshal.SizeOf<OriginalGuardRecord>() == OriginalRuntime.GuardRuntimeStride;

        public static void Reset()
        {
            Array.Clear(Objects, 0, Objects.Length);
            Array.Clear(Guards, 0, Guards.Length);
            GuardWakeCache.Clear();
            ObjectDefinitions.Clear();
            ProjectilePool.Clear();
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

        public static bool RegisterGuard(Entity entity, GuardType type, byte mapObjectId = 0)
        {
            byte objectClass = 0;
            byte variant = 0;
            bool hasMapSemantics = false;

            if (mapObjectId != 0 && Level.originalMap != null)
            {
                byte translatedClass = Level.originalMap.ObjectClass[mapObjectId];
                if (translatedClass != 0 &&
                    Level.originalMap.TryGetObjectClassAndVariant(
                        mapObjectId,
                        out objectClass,
                        out variant))
                {
                    hasMapSemantics = true;
                }
            }

            if (!hasMapSemantics &&
                !OriginalGuardProfiles.TryClassFromMapObjectId(
                    Game.episode, mapObjectId, out objectClass) &&
                !TryMapPortGuardClass(type, out objectClass))
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
            obj.MapObjectId = mapObjectId;
            obj.Variant = variant;
            obj.Flags = hasMapSemantics
                ? Level.originalMap.ObjectProperty[mapObjectId]
                : (byte)(RecoveredMechanics.ObjectRuntimePresent |
                         RecoveredMechanics.ObjectBlocksMovement |
                         RecoveredMechanics.ObjectCreatesGuard);
            obj.ObjectClass = objectClass;
            obj.GuardIndex = (byte)guardSlot;

            if (RegisterMapObjectDefinition(mapObjectId, out byte definitionId))
                obj.DefinitionId = definitionId;

            WriteWorldPosition(ref obj, entity.position);

            ref var guard = ref Guards[guardSlot];
            guard.ObjectSlot = (ushort)objectSlot;
            guard.Strength = OriginalRuntime.GuardInitialStrength;
            // FUN_847E initializes +0x0E to FF. FUN_247A later updates it only
            // while the actor is on wall class 0x44 AREA markers.
            guard.DefinitionLookup = 0xFF;

            var profile = OriginalGuardProfiles.ForObjectClass(objectClass);
            guard.Strategy = profile.Strategy;
            guard.State = profile.State;
            guard.NextState = profile.NextState;
            // FUN_1010_B02C writes this class-specific mode to GUARD +0x16.
            // FUN_1010_7594 later selects proximity (0) vs perception (1/2)
            // from the same byte during attack eligibility checks.
            guard.TransitionControl = profile.PerceptionMode;

            if (hasMapSemantics)
            {
                int tileX = obj.WorldX >> 6;
                int tileY = obj.WorldY >> 6;
                byte wallClass =
                    tileX >= 0 && tileY >= 0 &&
                    tileX < OriginalRuntime.MapWidth &&
                    tileY < OriginalRuntime.MapHeight
                    ? Level.originalMap.WallClassAt(tileX, tileY)
                    : (byte)0;

                OriginalGuardProfiles.ApplySpawnVariantAndCell(
                    ref guard,
                    variant,
                    wallClass);

                byte rawWallId = Level.originalMap.WallId[tileX, tileY];
                if (Level.originalMap.TryGetWallClassVariant(
                        rawWallId,
                        0x44,
                        out byte areaId))
                {
                    guard.DefinitionLookup = areaId;
                }
            }

            bindings[entity] = new Binding
            {
                ObjectSlot = objectSlot,
                GuardSlot = guardSlot
            };

            return true;
        }

        public static bool RegisterMapObjectDefinition(
            byte mapObjectId,
            out byte definitionId)
        {
            definitionId = 0;

            if (mapObjectId == 0 || Img.current == null)
                return false;

            return OriginalImgDefinitionLoader.TryRegisterObjectDefinition(
                Img.current.rawData,
                mapObjectId,
                ObjectDefinitions,
                out definitionId);
        }

        public static bool RegisterMapObjectDefinition(byte mapObjectId)
        {
            return RegisterMapObjectDefinition(mapObjectId, out _);
        }

        public static void SyncGuardPosition(Entity entity)
        {
            if (!bindings.TryGetValue(entity, out var binding) ||
                binding.ObjectSlot < 0)
            {
                return;
            }

            ref var obj = ref Objects[binding.ObjectSlot];
            WriteWorldPosition(ref obj, entity.position);

            if (binding.GuardSlot >= 0)
            {
                int tileX = obj.WorldX >> 6;
                int tileY = obj.WorldY >> 6;

                if (Level.originalMap != null &&
                    tileX >= 0 && tileY >= 0 &&
                    tileX < OriginalRuntime.MapWidth &&
                    tileY < OriginalRuntime.MapHeight)
                {
                    byte rawWallId = Level.originalMap.WallId[tileX, tileY];
                    if (Level.originalMap.TryGetWallClassVariant(
                            rawWallId,
                            0x44,
                            out byte areaId))
                    {
                        // FUN_247A/71DC deliberately preserve the previous area
                        // when the new cell is not a class-44 marker.
                        Guards[binding.GuardSlot].DefinitionLookup = areaId;
                    }
                }
            }
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

        public static bool EvaluateGuardPerception(
            Entity entity,
            bool secondaryCellChecks,
            bool ignoreFacing)
        {
            if (Game.player == null ||
                !bindings.TryGetValue(entity, out var binding) ||
                binding.GuardSlot < 0 ||
                binding.ObjectSlot < 0)
            {
                return false;
            }

            ref var guard = ref Guards[binding.GuardSlot];
            ref var obj = ref Objects[binding.ObjectSlot];

            short playerWorldX = ToWorldCoordinate(Game.player.position.X);
            short playerWorldY = ToWorldCoordinate(Game.player.position.Y);

            return OriginalGuardDispatcher.EvaluateGuardPerception(
                ref guard,
                ref obj,
                playerWorldX,
                playerWorldY,
                secondaryCellChecks,
                ignoreFacing,
                Level.OriginalPerceptionLineTrace);
        }

        public static bool TryGetGuardPackedSequence(
            Entity entity,
            OriginalGuardState state,
            out ushort packedSequence)
        {
            packedSequence = 0;

            if (!bindings.TryGetValue(entity, out var binding) ||
                binding.ObjectSlot < 0)
            {
                return false;
            }

            ref var obj = ref Objects[binding.ObjectSlot];
            return ObjectDefinitions.TryGetGuardStateWord(
                obj.DefinitionId,
                state,
                out packedSequence);
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
                case OriginalGuardState.AnimationTimer:
                    return OriginalGuardDispatcher.TickAnimationTimer(
                        ref guard, ref obj);

                case OriginalGuardState.Delay:
                    return OriginalGuardDispatcher.TickDelay(ref guard);

                case OriginalGuardState.WaitAnimation12:
                    return OriginalGuardDispatcher.TickWaitAnimation12(
                        ref guard, ref obj);

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

                case OriginalGuardState.Pain15:
                    return OriginalGuardDispatcher.TickPainReaction(
                        ref guard, ref obj);

                default:
                    return OriginalGuardDispatchResult.NotHandled;
            }
        }

        public static OriginalGuardDispatchResult PlanGuardMovement76FC(
            Entity entity)
        {
            return PlanGuardMovement76FC(entity, Difficulty);
        }

        public static OriginalGuardDispatchResult PlanGuardMovement76FC(
            Entity entity,
            byte difficulty)
        {
            if (Game.player == null ||
                !bindings.TryGetValue(entity, out var binding) ||
                binding.GuardSlot < 0 ||
                binding.ObjectSlot < 0)
            {
                return OriginalGuardDispatchResult.NotHandled;
            }

            ref var guard = ref Guards[binding.GuardSlot];
            ref var obj = ref Objects[binding.ObjectSlot];

            short playerWorldX = ToWorldCoordinate(Game.player.position.X);
            short playerWorldY = ToWorldCoordinate(Game.player.position.Y);

            bool doorFound = false;
            short doorTargetX = 0;
            short doorTargetY = 0;

            if (guard.Strategy == 1 && guard.Strength < 0x7F)
            {
                doorFound = Level.TryGetNearestRetreatDoorTarget(
                    obj.WorldX >> 6,
                    obj.WorldY >> 6,
                    out doorTargetX,
                    out doorTargetY);
            }

            return OriginalGuardDispatcher.PlanMovement76FC(
                ref guard,
                ref obj,
                playerWorldX,
                playerWorldY,
                difficulty,
                OriginalRandom.Next,
                doorFound,
                doorTargetX,
                doorTargetY);
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

        public static bool TryFindGuardHit(
            int projectileWorldX,
            int projectileWorldY,
            out Entity guardEntity)
        {
            foreach (var pair in bindings)
            {
                var binding = pair.Value;
                if (binding.GuardSlot < 0 ||
                    binding.ObjectSlot < 0 ||
                    Guards[binding.GuardSlot].Strength == 0)
                {
                    continue;
                }

                ref var obj = ref Objects[binding.ObjectSlot];
                if (OriginalProjectileRuntime.HitsGuard(
                        projectileWorldX,
                        projectileWorldY,
                        obj.WorldX,
                        obj.WorldY))
                {
                    guardEntity = pair.Key;
                    return true;
                }
            }

            guardEntity = null;
            return false;
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
            ReactionAnimation,
            ReactionSkipped,
            DeathSequenceStarted,
            MissingDefinition,
            SpecialReactionRequired,
            Killed,
            DraculaTransformed
        }

        /// <summary>
        /// Compatibility overload. The original selector is RNG-driven; callers
        /// requiring exact RNG-stream parity should use the overload that supplies
        /// nextRandom and the recovered perception result.
        /// </summary>
        public static GuardHitResult ApplyNormalGuardDamage(
            Entity entity,
            byte damage)
        {
            return ApplyNormalGuardDamage(
                entity,
                damage,
                () => 0,
                false);
        }

        /// <summary>
        /// Applies the recovered FUN_1010_80F8 receiver transition. This covers
        /// HP subtraction, resoct=8, strategy 2/4 handling, state-specific pain
        /// routing and lethal death-sequence setup. Class-specific state-09
        /// finalization is a later step and is intentionally separate.
        /// </summary>
        public static GuardHitResult ApplyNormalGuardDamage(
            Entity entity,
            byte damage,
            Func<ushort> nextRandom,
            bool perceptionSucceeded)
        {
            if (!bindings.TryGetValue(entity, out var binding) ||
                binding.GuardSlot < 0 ||
                binding.ObjectSlot < 0)
            {
                return GuardHitResult.NoRuntimeBinding;
            }

            if (damage == 0)
                return GuardHitResult.NoDamage;

            ref var guard = ref Guards[binding.GuardSlot];
            ref var obj = ref Objects[binding.ObjectSlot];

            if (damage >= guard.Strength)
            {
                if (!ObjectDefinitions.TryGetHeader(
                        obj.DefinitionId,
                        out var deathDefinition))
                {
                    guard.Strength = 0;
                    return GuardHitResult.MissingDefinition;
                }

                var transition =
                    OriginalGuardDispatcher.BeginLethalHitTransition(
                        ref guard,
                        ref obj,
                        deathDefinition,
                        nextRandom);

                return transition == OriginalGuardHitTransition.DeathAnimation
                    ? GuardHitResult.DeathSequenceStarted
                    : GuardHitResult.MissingDefinition;
            }

            guard.Strength = (byte)(guard.Strength - damage);

            // Strategy 4 exits before the reaction selector in the original.
            if (guard.Strategy == 4)
            {
                guard.ResultOctant = 8;
                return GuardHitResult.ReactionSkipped;
            }

            if (!ObjectDefinitions.TryGetHeader(
                    obj.DefinitionId,
                    out var reactionDefinition))
            {
                guard.ResultOctant = 8;
                return GuardHitResult.MissingDefinition;
            }

            var reaction =
                OriginalGuardDispatcher.BeginNonLethalHitTransition(
                    ref guard,
                    ref obj,
                    reactionDefinition,
                    nextRandom,
                    perceptionSucceeded);

            switch (reaction)
            {
                case OriginalGuardHitTransition.Pain15:
                    return GuardHitResult.PainReaction;

                case OriginalGuardHitTransition.AnimationTo05:
                case OriginalGuardHitTransition.AnimationTo08:
                    return GuardHitResult.ReactionAnimation;

                case OriginalGuardHitTransition.ReactionSkipped:
                    return GuardHitResult.ReactionSkipped;

                default:
                    return GuardHitResult.MissingDefinition;
            }
        }

        static bool TryFindDefinitionIdForObjectClass(
            byte objectClass,
            out byte definitionId)
        {
            for (int i = 0; i < ObjectCount; i++)
            {
                if (Objects[i].ObjectClass == objectClass)
                {
                    definitionId = Objects[i].DefinitionId;
                    return true;
                }
            }

            definitionId = 0;
            return false;
        }

        /// <summary>
        /// Recovered state-0x09 FUN_1010_A0EE death finalization core.
        /// Ordinary class side effects are limited to the confirmed OBJECT flag
        /// writes. Draculas phase change is performed here, after its death
        /// animation, rather than at lethal-hit time.
        /// </summary>
        public static GuardHitResult FinalizeDeath09(Entity entity)
        {
            if (!bindings.TryGetValue(entity, out var binding) ||
                binding.GuardSlot < 0 ||
                binding.ObjectSlot < 0)
            {
                return GuardHitResult.NoRuntimeBinding;
            }

            ref var guard = ref Guards[binding.GuardSlot];
            ref var obj = ref Objects[binding.ObjectSlot];

            if (guard.State != (byte)OriginalGuardState.DeathFinalize09)
                return GuardHitResult.SpecialReactionRequired;

            obj.Flags |= 0x01;
            guard.State = (byte)OriginalGuardState.NoLocalAction0A;

            switch (obj.ObjectClass)
            {
                // These FUN_A0EE cases clear the bit it sets on entry.
                case 0x09:
                case 0x0A:
                case 0x12:
                case 0x13:
                case 0x1A:
                case 0x1E:
                case 0x1F:
                    obj.Flags &= 0xFE;
                    return GuardHitResult.Killed;

                case OriginalRuntime.DraculaPhase1Class:
                    // FUN_1010_23DC(8) finds the first OBJECT class 0x08 and
                    // returns its OBJECT+0x04 definition id for the Bat resource.
                    if (!TryFindDefinitionIdForObjectClass(
                            0x08,
                            out byte batDefinitionId))
                    {
                        return GuardHitResult.SpecialReactionRequired;
                    }

                    OriginalGuardDispatcher.ApplyDraculaPhase2Reset(
                        ref guard,
                        ref obj,
                        batDefinitionId);
                    return GuardHitResult.DraculaTransformed;

                case OriginalRuntime.DrHamersteinClass:
                    // The same function also mutates end-game globals for
                    // Hamerstein. Keep that global side effect outside this
                    // bridge until those globals are represented explicitly.
                    return GuardHitResult.SpecialReactionRequired;

                default:
                    return GuardHitResult.Killed;
            }
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
