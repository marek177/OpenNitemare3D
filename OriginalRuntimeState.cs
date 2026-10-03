using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Nitemare3D
{
    /// <summary>
    /// Production-side storage for the recovered fixed Win16 OBJECT/GUARD pools.
    /// The legacy Entity classes remain the default/fallback shell, while the
    /// opt-in autonomous path lets confirmed original GUARD states own gameplay
    /// state and movement at the recovered 125-ms logic cadence.
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

        // FUN_D70A/D9C6: gameplay/GUARD update clock is floor(ms*8/1000),
        // i.e. one global logic step every 125 ms. Extra missed intervals are
        // not replayed as catch-up ticks.
        public const float GuardLogicTickSeconds = 0.125f;
        static float guardLogicAccumulator;
        public static bool GuardLogicTickDue { get; private set; }

        // Opt-in while the remaining rare GUARD states are still being closed.
        public static bool AutonomousGuardRuntimeEnabled { get; set; }

        // DAT_1048_4BE7 shared processing gate. No port-side producer is known
        // yet, so the default remains false; exposing it preserves the branch.
        public static bool GuardProcessingGate { get; set; }

        // DAT_1048_51A5. FUN_0EF6 initializes it to 1 and the Win16 UI can
        // toggle it. Keep the neutral address-backed name until the UI label is
        // independently identified. Unlike per-level state, it survives Reset().
        public static bool GuardState0E10Gate51A5 { get; set; } = true;

        // DAT_1048_4C1C. FUN_8A20 updates this only when the player's current
        // wall belongs to semantic class 0x44 (AREA marker), so the last value
        // persists while crossing ordinary cells.
        public static byte PlayerAreaSelector { get; private set; }

        // DAT_1048_53DC. The original increments this 32-bit generation before
        // world-object projection. GUARD +0x02..05 receives the value only when
        // the projected sprite overlaps the aim center; hitscan requires equality.
        public static uint CurrentRenderGeneration { get; private set; }

        // Mirrors the second half of the class-0x16 damage gate:
        // (episode == 3 || DAT_1048_51A6 != 0). The exact producer of 51A6 is
        // still separate, so production defaults this override to false.
        public static bool GuardAttackClass16FullDamageOverride { get; set; }

        public static void BeginFrame(float deltaSeconds)
        {
            GuardLogicTickDue = false;
            RefreshPlayerAreaSelectorFromMap();

            if (!AutonomousGuardRuntimeEnabled || deltaSeconds <= 0)
                return;

            guardLogicAccumulator += deltaSeconds;
            if (guardLogicAccumulator < GuardLogicTickSeconds)
                return;

            GuardLogicTickDue = true;

            // D70A observes the current 125-ms bin rather than replaying every
            // skipped bin. Preserve phase but drop catch-up iterations.
            guardLogicAccumulator %= GuardLogicTickSeconds;
        }

        static void RefreshPlayerAreaSelectorFromMap()
        {
            if (Game.player == null || Level.originalMap == null)
                return;

            int tileX = (int)Game.player.position.X;
            int tileY = (int)Game.player.position.Y;

            if (tileX < 0 || tileY < 0 ||
                tileX >= OriginalRuntime.MapWidth ||
                tileY >= OriginalRuntime.MapHeight)
            {
                return;
            }

            byte rawWallId = Level.originalMap.WallId[tileX, tileY];
            if (Level.originalMap.TryGetWallClassVariant(
                    rawWallId,
                    0x44,
                    out byte areaId))
            {
                PlayerAreaSelector = areaId;
            }
        }

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
            GuardAttackClass16FullDamageOverride = false;
            guardLogicAccumulator = 0;
            GuardLogicTickDue = false;
            GuardProcessingGate = false;
            CurrentRenderGeneration = 0;
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

        public static int WakeGuardsAfterPlayerFire(byte areaId)
        {
            return WakeGuardsAfterPlayerFire(
                areaId,
                i => OriginalRandom.Next());
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

        static void ConsumeOriginalAlertSoundSelection(byte objectClass)
        {
            ushort randomValue = OriginalGuardSounds.AlertUsesRandom(objectClass)
                ? OriginalRandom.Next()
                : (ushort)0;

            // Keep RNG parity now; route the selected original SND index through
            // the corrected audio bridge once the legacy soundOffset path is fixed.
            OriginalGuardSounds.AlertSoundId(objectClass, randomValue);
        }

        static void ConsumeOriginalAttackSoundSelection(byte objectClass)
        {
            ushort randomValue = OriginalGuardSounds.AttackUsesRandom(objectClass)
                ? OriginalRandom.Next()
                : (ushort)0;

            OriginalGuardSounds.AttackSoundId(objectClass, randomValue);
        }

        static bool TryEvaluateAttackEligibility(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj,
            out bool attackEligible)
        {
            attackEligible = false;
            if (Game.player == null)
                return false;

            short playerWorldX = ToWorldCoordinate(Game.player.position.X);
            short playerWorldY = ToWorldCoordinate(Game.player.position.Y);

            // FUN_7594 invokes FUN_7494 with both control arguments set to 1:
            // secondary object-cell checks enabled and facing/FOV bypassed.
            bool perceptionSucceeded =
                OriginalGuardDispatcher.EvaluateGuardPerception(
                    ref guard,
                    ref obj,
                    playerWorldX,
                    playerWorldY,
                    true,
                    true,
                    Level.OriginalPerceptionLineTrace);

            return OriginalGuardDispatcher.TryEvaluateAttackGate(
                ref guard,
                ref obj,
                playerWorldX,
                playerWorldY,
                perceptionSucceeded,
                out attackEligible);
        }

        static bool ApplyGuardAttackDamageToPlayer(
            ref OriginalObjectRecord obj)
        {
            if (Game.player == null)
                return false;

            short playerWorldX = ToWorldCoordinate(Game.player.position.X);
            short playerWorldY = ToWorldCoordinate(Game.player.position.Y);

            ushort randomValue =
                OriginalDamage.GuardAttackUsesRandom(obj.ObjectClass)
                    ? OriginalRandom.Next()
                    : (ushort)0;

            bool class16FullDamageGate =
                Game.episode == 3 ||
                GuardAttackClass16FullDamageOverride;

            var damage = OriginalDamage.ComputeGuardToPlayerFromWorld(
                obj.WorldX,
                obj.WorldY,
                playerWorldX,
                playerWorldY,
                obj.ObjectClass,
                Difficulty,
                class16FullDamageGate,
                randomValue);

            int hp = Game.player.health;
            int amount = damage.DifficultyTransformed;
            Game.player.health = amount >= hp ? 0 : hp - amount;
            return Game.player.health <= 0;
        }

        static bool EvaluateGuardMovementBlocked(
            int guardSlot,
            int objectSlot,
            OriginalObjectDefinitionRecord definition,
            short playerWorldX,
            short playerWorldY,
            short candidateWorldX,
            short candidateWorldY)
        {
            int tileX = candidateWorldX >> 6;
            int tileY = candidateWorldY >> 6;

            if (!Level.TryResolveOriginalMovementCell(
                    tileX,
                    tileY,
                    out byte wallFlags,
                    out byte objectFlags,
                    out OriginalDoorCollisionInfo door))
            {
                return true;
            }

            ref var guard = ref Guards[guardSlot];
            ref var obj = ref Objects[objectSlot];

            var collision =
                OriginalGuardDispatcher.EvaluateMovementCell700A(
                    ref guard,
                    ref obj,
                    candidateWorldX,
                    candidateWorldY,
                    playerWorldX,
                    playerWorldY,
                    wallFlags,
                    objectFlags,
                    door,
                    definition);

            if (collision.DoorToggleRequested)
            {
                Level.ApplyOriginalGuardDoorInteraction(
                    tileX,
                    tileY,
                    collision.DoorLatchRequested,
                    guard.Octant);
            }

            return collision.Blocked;
        }

        static OriginalGuardDispatchResult TickState06Bridge(
            Entity entity,
            Binding binding,
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj)
        {
            if (Game.player == null ||
                !ObjectDefinitions.TryGetHeader(
                    obj.DefinitionId,
                    out var definition))
            {
                return OriginalGuardDispatchResult.NotHandled;
            }

            short playerWorldX = ToWorldCoordinate(Game.player.position.X);
            short playerWorldY = ToWorldCoordinate(Game.player.position.Y);
            int guardSlot = binding.GuardSlot;
            int objectSlot = binding.ObjectSlot;

            var result = OriginalGuardDispatcher.TickState06Movement(
                ref guard,
                ref obj,
                definition,
                playerWorldX,
                playerWorldY,
                (candidateX, candidateY) =>
                    EvaluateGuardMovementBlocked(
                        guardSlot,
                        objectSlot,
                        definition,
                        playerWorldX,
                        playerWorldY,
                        candidateX,
                        candidateY),
                OriginalRandom.Next,
                out _);

            entity.position.X =
                (float)obj.WorldX / OriginalRuntime.WorldUnitsPerTile;
            entity.position.Y =
                (float)obj.WorldY / OriginalRuntime.WorldUnitsPerTile;
            SyncGuardPosition(entity);

            return result;
        }

        static OriginalGuardDispatchResult TickState11Bridge(
            Entity entity,
            Binding binding,
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj)
        {
            if (Game.player == null ||
                !ObjectDefinitions.TryGetHeader(
                    obj.DefinitionId,
                    out var definition))
            {
                return OriginalGuardDispatchResult.NotHandled;
            }

            short playerWorldX = ToWorldCoordinate(Game.player.position.X);
            short playerWorldY = ToWorldCoordinate(Game.player.position.Y);
            int guardSlot = binding.GuardSlot;
            int objectSlot = binding.ObjectSlot;

            var result = OriginalGuardDispatcher.TickState11Movement(
                ref guard,
                ref obj,
                definition,
                playerWorldX,
                playerWorldY,
                (candidateX, candidateY) =>
                    EvaluateGuardMovementBlocked(
                        guardSlot,
                        objectSlot,
                        definition,
                        playerWorldX,
                        playerWorldY,
                        candidateX,
                        candidateY),
                OriginalRandom.Next,
                out _);

            entity.position.X =
                (float)obj.WorldX / OriginalRuntime.WorldUnitsPerTile;
            entity.position.Y =
                (float)obj.WorldY / OriginalRuntime.WorldUnitsPerTile;
            SyncGuardPosition(entity);

            return result;
        }

        static OriginalGuardDispatchResult TickDormantDirectionalBridge(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj)
        {
            if (Game.player == null ||
                !ObjectDefinitions.TryGetHeader(
                    obj.DefinitionId,
                    out var definition))
            {
                // 0C/0D are retail-dormant compatibility states. Preserve the
                // injected/restored state rather than falling into guessed AI.
                return OriginalGuardDispatchResult.Waiting;
            }

            short playerWorldX =
                ToWorldCoordinate(Game.player.position.X);
            short playerWorldY =
                ToWorldCoordinate(Game.player.position.Y);

            // FUN_7E54 is shared by 0C/0D and only forces the normal
            // directional/sequence refresh; it does not leave the state.
            return OriginalGuardDispatcher.RefreshDirectionalSequence(
                ref guard,
                ref obj,
                definition,
                playerWorldX,
                playerWorldY,
                true);
        }

        static OriginalGuardDispatchResult TickState08Bridge(
            Entity entity,
            Binding binding,
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj)
        {
            if (Game.player == null ||
                Level.originalMap == null ||
                !ObjectDefinitions.TryGetHeader(
                    obj.DefinitionId,
                    out var definition))
            {
                return OriginalGuardDispatchResult.NotHandled;
            }

            short playerWorldX = ToWorldCoordinate(Game.player.position.X);
            short playerWorldY = ToWorldCoordinate(Game.player.position.Y);

            int tileX = obj.WorldX >> 6;
            int tileY = obj.WorldY >> 6;

            if (tileX >= 0 && tileY >= 0 &&
                tileX < OriginalRuntime.MapWidth &&
                tileY < OriginalRuntime.MapHeight)
            {
                byte rawWallId = Level.originalMap.WallId[tileX, tileY];
                byte wallClass = Level.originalMap.WallClass[rawWallId];
                byte wallVariant = 0;

                if ((wallClass == 0x41 || wallClass == 0x42) &&
                    Level.originalMap.TryGetWallClassVariant(
                        rawWallId,
                        wallClass,
                        out byte variant))
                {
                    wallVariant = variant;
                }

                OriginalGuardDispatcher.ApplyState08WallTurn(
                    ref guard,
                    ref obj,
                    wallClass,
                    wallVariant);
            }

            int guardSlot = binding.GuardSlot;
            int objectSlot = binding.ObjectSlot;

            var movement =
                OriginalGuardDispatcher.TickMovementCollisionCore(
                    ref guard,
                    ref obj,
                    (candidateX, candidateY) =>
                        EvaluateGuardMovementBlocked(
                            guardSlot,
                            objectSlot,
                            definition,
                            playerWorldX,
                            playerWorldY,
                            candidateX,
                            candidateY),
                    OriginalRandom.Next);

            OriginalGuardDispatcher.RefreshDirectionalSequence(
                ref guard,
                ref obj,
                definition,
                playerWorldX,
                playerWorldY,
                false);

            entity.position.X =
                (float)obj.WorldX / OriginalRuntime.WorldUnitsPerTile;
            entity.position.Y =
                (float)obj.WorldY / OriginalRuntime.WorldUnitsPerTile;

            if (guard.NextState == (byte)OriginalGuardState.Active02 &&
                !GuardProcessingGate)
            {
                bool perceived =
                    OriginalGuardDispatcher.EvaluateGuardPerception(
                        ref guard,
                        ref obj,
                        playerWorldX,
                        playerWorldY,
                        false,
                        false,
                        Level.OriginalPerceptionLineTrace);

                if (perceived)
                {
                    guard.State = (byte)OriginalGuardState.Active02;
                    return OriginalGuardDispatchResult.Transitioned;
                }
            }

            if (movement.AppliedX != 0 || movement.AppliedY != 0)
                return movement.PositionCommitted
                    ? OriginalGuardDispatchResult.Moved
                    : OriginalGuardDispatchResult.MovementBlocked;

            return movement.XBlocked || movement.YBlocked
                ? OriginalGuardDispatchResult.MovementBlocked
                : OriginalGuardDispatchResult.Waiting;
        }

        static int ReleaseState14Group()
        {
            int released = 0;

            for (int i = 0; i < GuardCount; i++)
            {
                ref var member = ref Guards[i];
                if (member.State != (byte)OriginalGuardState.Periodic14)
                    continue;

                int objectSlot = member.ObjectSlot;
                if (objectSlot < 0 || objectSlot >= ObjectCount)
                    continue;

                ref var memberObject = ref Objects[objectSlot];
                byte savedDefinitionId = member.NextState;

                ushort directionalC0 = 0;
                if (ObjectDefinitions.TryGetHeader(
                        savedDefinitionId,
                        out var restoredDefinition))
                {
                    directionalC0 = restoredDefinition.DirectionalC0;
                }

                if (OriginalGuardDispatcher.ReleaseState14Record(
                        ref member,
                        ref memberObject,
                        directionalC0))
                {
                    released++;
                }
            }

            // Original AE56(1) also emits SND 0x45 and restores normal level
            // music through C63E. The current port already owns background music,
            // while the legacy SND bridge is not safe for original IDs <34.
            return released;
        }

        static OriginalGuardDispatchResult TickState14Bridge(
            Entity entity,
            Binding binding,
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj)
        {
            var countdown = OriginalGuardDispatcher.TickState14Countdown(
                ref guard,
                out bool movementDue,
                out bool releaseGroup);

            if (countdown == OriginalGuardDispatchResult.NotHandled)
                return countdown;

            if (releaseGroup)
            {
                return ReleaseState14Group() > 0
                    ? OriginalGuardDispatchResult.Transitioned
                    : OriginalGuardDispatchResult.Waiting;
            }

            if (!movementDue)
                return countdown;

            if (Game.player == null ||
                !ObjectDefinitions.TryGetHeader(
                    obj.DefinitionId,
                    out var definition))
            {
                return OriginalGuardDispatchResult.NotHandled;
            }

            short playerWorldX =
                ToWorldCoordinate(Game.player.position.X);
            short playerWorldY =
                ToWorldCoordinate(Game.player.position.Y);

            int guardSlot = binding.GuardSlot;
            int objectSlot = binding.ObjectSlot;

            var movement = OriginalGuardDispatcher.TickMovementCollisionCore(
                ref guard,
                ref obj,
                (candidateX, candidateY) =>
                    EvaluateGuardMovementBlocked(
                        guardSlot,
                        objectSlot,
                        definition,
                        playerWorldX,
                        playerWorldY,
                        candidateX,
                        candidateY),
                OriginalRandom.Next);

            entity.position.X =
                (float)obj.WorldX / OriginalRuntime.WorldUnitsPerTile;
            entity.position.Y =
                (float)obj.WorldY / OriginalRuntime.WorldUnitsPerTile;
            SyncGuardPosition(entity);

            if (movement.AppliedX != 0 || movement.AppliedY != 0)
                return movement.PositionCommitted
                    ? OriginalGuardDispatchResult.Moved
                    : OriginalGuardDispatchResult.MovementBlocked;

            return movement.XBlocked || movement.YBlocked
                ? OriginalGuardDispatchResult.MovementBlocked
                : OriginalGuardDispatchResult.Waiting;
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

                case OriginalGuardState.Active02:
                {
                    if (!ObjectDefinitions.TryGetGuardStateWord(
                            obj.DefinitionId,
                            OriginalGuardState.Active02,
                            out ushort alertSequence))
                    {
                        return OriginalGuardDispatchResult.NotHandled;
                    }

                    ConsumeOriginalAlertSoundSelection(obj.ObjectClass);
                    return OriginalGuardDispatcher.BeginState02AlertSequence(
                        ref guard,
                        ref obj,
                        alertSequence);
                }

                case OriginalGuardState.Detection03:
                {
                    if (!TryEvaluateAttackEligibility(
                            ref guard,
                            ref obj,
                            out bool attackEligible))
                    {
                        return OriginalGuardDispatchResult.NotHandled;
                    }

                    if (!attackEligible)
                        return PlanGuardMovement76FC(entity);

                    if (!ObjectDefinitions.TryGetGuardStateWord(
                            obj.DefinitionId,
                            OriginalGuardState.Detection03,
                            out ushort attackSequence))
                    {
                        return OriginalGuardDispatchResult.NotHandled;
                    }

                    return OriginalGuardDispatcher.BeginState03AttackSequence(
                        ref guard,
                        ref obj,
                        attackSequence);
                }

                case OriginalGuardState.DetectionAttack04:
                {
                    // Once the player death gate is active the original leaves
                    // state 04 without scheduling the +0x38 recovery sequence.
                    if (Game.player == null || Game.player.health <= 0)
                        return OriginalGuardDispatchResult.Waiting;

                    if (!TryEvaluateAttackEligibility(
                            ref guard,
                            ref obj,
                            out bool attackEligible))
                    {
                        return OriginalGuardDispatchResult.NotHandled;
                    }

                    if (attackEligible)
                    {
                        ConsumeOriginalAttackSoundSelection(obj.ObjectClass);

                        if (ApplyGuardAttackDamageToPlayer(ref obj))
                            return OriginalGuardDispatchResult.Waiting;
                    }

                    if (!ObjectDefinitions.TryGetGuardStateWord(
                            obj.DefinitionId,
                            OriginalGuardState.DetectionAttack04,
                            out ushort recoverySequence))
                    {
                        return OriginalGuardDispatchResult.NotHandled;
                    }

                    return OriginalGuardDispatcher.BeginState04RecoverySequence(
                        ref guard,
                        ref obj,
                        recoverySequence);
                }

                case OriginalGuardState.Transition05:
                    return PlanGuardMovement76FC(entity);

                case OriginalGuardState.MoveThen03:
                    return TickState06Bridge(
                        entity,
                        binding,
                        ref guard,
                        ref obj);

                case OriginalGuardState.Active07:
                {
                    if (Game.player == null)
                        return OriginalGuardDispatchResult.NotHandled;

                    if (!ObjectDefinitions.TryGetHeader(
                            obj.DefinitionId,
                            out var definition))
                    {
                        return OriginalGuardDispatchResult.NotHandled;
                    }

                    short playerWorldX =
                        ToWorldCoordinate(Game.player.position.X);
                    short playerWorldY =
                        ToWorldCoordinate(Game.player.position.Y);

                    // FUN_7B56 state 07 calls FUN_6EE0 before the processing
                    // gate and FUN_7494 perception test.
                    OriginalGuardDispatcher.RefreshDirectionalSequence(
                        ref guard,
                        ref obj,
                        definition,
                        playerWorldX,
                        playerWorldY,
                        false);

                    if (GuardProcessingGate)
                        return OriginalGuardDispatchResult.Waiting;

                    bool perceived =
                        OriginalGuardDispatcher.EvaluateGuardPerception(
                            ref guard,
                            ref obj,
                            playerWorldX,
                            playerWorldY,
                            false,
                            false,
                            Level.OriginalPerceptionLineTrace);

                    ushort randomValue =
                        perceived && guard.Strategy == 3
                            ? OriginalRandom.Next()
                            : (ushort)0;

                    return OriginalGuardDispatcher.ResolveState07Perception(
                        ref guard,
                        GuardProcessingGate,
                        perceived,
                        randomValue);
                }

                case OriginalGuardState.Move08:
                    return TickState08Bridge(
                        entity,
                        binding,
                        ref guard,
                        ref obj);

                case OriginalGuardState.DeathFinalize09:
                {
                    var finalized = FinalizeDeath09(entity);

                    if (finalized == GuardHitResult.DraculaTransformed)
                        return OriginalGuardDispatchResult.Transitioned;

                    if (finalized == GuardHitResult.Killed)
                    {
                        if (entity is Guard legacyGuard)
                            legacyGuard.visible = false;

                        Entity.Remove(entity);
                        return OriginalGuardDispatchResult.Completed;
                    }

                    return OriginalGuardDispatchResult.NotHandled;
                }

                case OriginalGuardState.NoLocalAction0A:
                case OriginalGuardState.LethalPlayerContact0B:
                    // Both values share the original no-local-action dispatcher
                    // target. Keeping them handled prevents legacy AI resurrection.
                    return OriginalGuardDispatchResult.Waiting;

                case OriginalGuardState.Shared0C:
                case OriginalGuardState.Shared0D:
                    return TickDormantDirectionalBridge(
                        ref guard,
                        ref obj);

                case OriginalGuardState.Conditional0E:
                {
                    if (Game.player == null ||
                        !ObjectDefinitions.TryGetHeader(
                            obj.DefinitionId,
                            out var definition))
                    {
                        return OriginalGuardDispatchResult.NotHandled;
                    }

                    short playerWorldX =
                        ToWorldCoordinate(Game.player.position.X);
                    short playerWorldY =
                        ToWorldCoordinate(Game.player.position.Y);

                    return OriginalGuardDispatcher.TickState0E(
                        ref guard,
                        ref obj,
                        definition,
                        playerWorldX,
                        playerWorldY,
                        GuardState0E10Gate51A5);
                }

                case OriginalGuardState.Timed0F:
                {
                    if (Game.player == null ||
                        !ObjectDefinitions.TryGetHeader(
                            obj.DefinitionId,
                            out var definition))
                    {
                        return OriginalGuardDispatchResult.NotHandled;
                    }

                    short playerWorldX =
                        ToWorldCoordinate(Game.player.position.X);
                    short playerWorldY =
                        ToWorldCoordinate(Game.player.position.Y);

                    var result = OriginalGuardDispatcher.TickState0F(
                        ref guard,
                        ref obj,
                        definition,
                        playerWorldX,
                        playerWorldY,
                        GuardState0E10Gate51A5,
                        guard.DefinitionLookup == PlayerAreaSelector,
                        out bool attackSoundRequested);

                    if (attackSoundRequested)
                        ConsumeOriginalAttackSoundSelection(obj.ObjectClass);

                    return result;
                }

                case OriginalGuardState.Timed10:
                {
                    if (Game.player == null ||
                        !ObjectDefinitions.TryGetHeader(
                            obj.DefinitionId,
                            out var definition))
                    {
                        return OriginalGuardDispatchResult.NotHandled;
                    }

                    short playerWorldX =
                        ToWorldCoordinate(Game.player.position.X);
                    short playerWorldY =
                        ToWorldCoordinate(Game.player.position.Y);

                    if (!OriginalGuardDispatcher.State10AttackDue(
                            ref guard,
                            ref obj,
                            definition,
                            playerWorldX,
                            playerWorldY))
                    {
                        return OriginalGuardDispatchResult.Waiting;
                    }

                    if (!TryEvaluateAttackEligibility(
                            ref guard,
                            ref obj,
                            out bool attackEligible))
                    {
                        return OriginalGuardDispatchResult.NotHandled;
                    }

                    if (attackEligible)
                        ApplyGuardAttackDamageToPlayer(ref obj);

                    return OriginalGuardDispatcher.CompleteState10AttackCycle(
                        ref guard,
                        ref obj,
                        definition,
                        playerWorldX,
                        playerWorldY);
                }

                case OriginalGuardState.RecoverMove11:
                    return TickState11Bridge(
                        entity,
                        binding,
                        ref guard,
                        ref obj);

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

                case OriginalGuardState.Periodic14:
                    return TickState14Bridge(
                        entity,
                        binding,
                        ref guard,
                        ref obj);

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

        public static bool TryGetGuardRuntimeState(
            Entity entity,
            out OriginalGuardState state,
            out OriginalGuardState nextState)
        {
            state = default;
            nextState = default;

            if (!bindings.TryGetValue(entity, out var binding) ||
                binding.GuardSlot < 0)
            {
                return false;
            }

            ref var guard = ref Guards[binding.GuardSlot];
            state = (OriginalGuardState)guard.State;
            nextState = (OriginalGuardState)guard.NextState;
            return true;
        }

        public static bool IsConfirmedAutonomousState(OriginalGuardState state)
        {
            switch (state)
            {
                case OriginalGuardState.AnimationTimer:
                case OriginalGuardState.Delay:
                case OriginalGuardState.Active02:
                case OriginalGuardState.Detection03:
                case OriginalGuardState.DetectionAttack04:
                case OriginalGuardState.Transition05:
                case OriginalGuardState.MoveThen03:
                case OriginalGuardState.Active07:
                case OriginalGuardState.Move08:
                case OriginalGuardState.DeathFinalize09:
                case OriginalGuardState.NoLocalAction0A:
                case OriginalGuardState.LethalPlayerContact0B:
                case OriginalGuardState.Shared0C:
                case OriginalGuardState.Shared0D:
                case OriginalGuardState.Conditional0E:
                case OriginalGuardState.Timed0F:
                case OriginalGuardState.Timed10:
                case OriginalGuardState.RecoverMove11:
                case OriginalGuardState.WaitAnimation12:
                case OriginalGuardState.Transition13:
                case OriginalGuardState.Periodic14:
                case OriginalGuardState.Pain15:
                    return true;

                default:
                    return false;
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

        public static void BeginRenderGeneration()
        {
            unchecked
            {
                CurrentRenderGeneration++;
            }

            // A zero stamp is also the spawn/reset value. Skip it on wrap so a
            // never-projected guard cannot accidentally look current.
            if (CurrentRenderGeneration == 0)
                CurrentRenderGeneration = 1;
        }

        public static bool RecordGuardProjection(
            Entity entity,
            short projectedBaseRow,
            bool overlapsAimCenter)
        {
            if (!bindings.TryGetValue(entity, out var binding) ||
                binding.GuardSlot < 0 ||
                binding.ObjectSlot < 0)
            {
                return false;
            }

            ref var obj = ref Objects[binding.ObjectSlot];
            ref var guard = ref Guards[binding.GuardSlot];

            // OBJECT+0x18 is persistent: projection overwrites it only after a
            // successful visible sprite projection; failed/off-screen frames do
            // not clear the previous cached row.
            obj.ProjectedBaseRow = projectedBaseRow;

            if (overlapsAimCenter)
                guard.Timestamp = CurrentRenderGeneration;

            return true;
        }

        public static bool GuardHasCurrentAimStamp(Entity entity)
        {
            return bindings.TryGetValue(entity, out var binding) &&
                   binding.GuardSlot >= 0 &&
                   CurrentRenderGeneration != 0 &&
                   Guards[binding.GuardSlot].Timestamp ==
                       CurrentRenderGeneration;
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
