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

        public static readonly OriginalWeaponRuntime WeaponRuntime =
            new OriginalWeaponRuntime();

        public static readonly OriginalShadeRuntime ShadeRuntime =
            new OriginalShadeRuntime();

        public static readonly OriginalPickupRuntime PickupRuntime =
            new OriginalPickupRuntime();

        static readonly Dictionary<Entity, Binding> bindings =
            new Dictionary<Entity, Binding>();

        public static int ObjectCount { get; private set; }
        public static int GuardCount { get; private set; }

        // DAT_1048_4C14. FUN_BA16 initializes this to 1 (normal); the
        // difficulty menu writes 0/1/2. It intentionally survives level Reset().
        public static byte Difficulty { get; private set; } =
            OriginalRuntime.DefaultDifficulty;

        // DAT_1048_4C16. FUN_1010_80F8 sign-extends the class-specific
        // kill-score result and adds it here on the lethal GUARD path.
        // Level Reset() intentionally does not clear it.
        public static int PlayerScore { get; private set; }

        // FUN_D70A/D9C6: gameplay/GUARD update clock is floor(ms*8/1000),
        // i.e. one global logic step every 125 ms. Extra missed intervals are
        // not replayed as catch-up ticks.
        public const float GuardLogicTickSeconds = 0.125f;

        // FUN_D7D0 calibration outputs. The portable default corresponds to the
        // original minimum effective render duration D=40 ms; a startup timing
        // bridge can replace RawFrameMeanMs later without changing projectile code.
        public static ushort RawFrameMeanMs { get; private set; } = 40;
        public static ushort FrameRateParameter53F4 { get; private set; } = 25;
        public static ushort MovementStep53F6 { get; private set; } = 10;
        public static ushort TurnStep53F8 { get; private set; } = 5;
        public static int ProjectileSubstepsPerTick { get; private set; } = 20;

        public static float ProjectileLogicTickSeconds =>
            FrameRateParameter53F4 == 0
                ? 0.040f
                : 1.0f / FrameRateParameter53F4;

        static float guardLogicAccumulator;
        static float projectileLogicAccumulator;
        static double runtimeClockFractionMs;

        public static uint RuntimeClockMs { get; private set; }

        public static OriginalTrigQ10 ExactTrigQ10 { get; private set; }

        public static bool ProjectileDefinitionsReady { get; private set; }
        static byte projectilePlasmaFlightDefinition;
        static byte projectilePlasmaImpactDefinition;
        static byte projectileMagicFlightDefinition;
        static byte projectileMagicImpactDefinition;

        public static bool SlowLogicTickDue { get; private set; }
        public static bool GuardLogicTickDue { get; private set; }
        public static bool ProjectileLogicTickDue { get; private set; }

        // All recovered GUARD states 0x00..0x15 now have production routing.
        // Keep the original-runtime path enabled by default; callers may still
        // disable it explicitly for A/B comparison against the legacy port AI.
        public static bool AutonomousGuardRuntimeEnabled { get; set; } = true;

        // DAT_1048_4BE7. Win16 menu/config command 0x24 reads/writes this byte,
        // status serialization emits the letter 'I', and GUARD states 07/08 skip
        // perception while it is nonzero. Treat it as the original player
        // invisibility flag.
        public static bool PlayerInvisible { get; set; }

        // Compatibility alias for code written before the 4BE7 semantic closure.
        public static bool GuardProcessingGate
        {
            get => PlayerInvisible;
            set => PlayerInvisible = value;
        }

        // DAT_1048_51A4. FUN_0EF6 initializes it to zero. Win16 menu
        // items 0x1E/0x1F are literally "Open remote doors" / "Close remote doors".
        // Each bit records the open/closed selection for one area/group selector.
        // Like 51A5, this is session/script state rather than a per-level pool field.
        public static byte RemoteDoorsOpenMask { get; private set; }

        public static bool RemoteDoorsOpenForArea(byte areaSelector)
        {
            if (areaSelector >= 8)
                return false;

            return (RemoteDoorsOpenMask & (1 << areaSelector)) != 0;
        }

        public static int ApplyRemoteDoorCommand(
            byte areaSelector,
            bool open)
        {
            int changed = Level.originalWalls != null
                ? Level.originalWalls.ApplyRemoteDoorCommand(
                    areaSelector,
                    open)
                : 0;

            if (areaSelector < 8)
            {
                byte bit = (byte)(1 << areaSelector);
                RemoteDoorsOpenMask = open
                    ? (byte)(RemoteDoorsOpenMask | bit)
                    : (byte)(RemoteDoorsOpenMask & ~bit);
            }

            return changed;
        }

        // DAT_1048_51A5. FUN_0EF6 initializes it to 1. Win16 menu commands
        // 0x20/0x21 are literally "Enable remote cannons" / "Disable remote cannons"
        // and toggle this byte. Cannon class 0x19 states 0E/0F/10 consume it.
        public static bool RemoteCannonsEnabled { get; set; } = true;

        // Compatibility alias retained for code written before the UI-label closure.
        public static bool GuardState0E10Gate51A5
        {
            get => RemoteCannonsEnabled;
            set => RemoteCannonsEnabled = value;
        }

        // DAT_1048_4C1C. FUN_8A20 updates this only when the player's current
        // wall belongs to semantic class 0x44 (AREA marker), so the last value
        // persists while crossing ordinary cells.
        public static byte PlayerAreaSelector { get; private set; }

        // DAT_1048_53DC. The original increments this 32-bit generation before
        // world-object projection. GUARD +0x02..05 receives the value only when
        // the projected sprite overlaps the aim center; hitscan requires equality.
        public static uint CurrentRenderGeneration { get; private set; }

        // DAT_1048_51A6. This is a persistent scripted-progression byte,
        // initialized to 0 by FUN_0EF6 and promoted by FUN_BFD8 event handlers.
        // Class-0x16 enemy damage tests only whether it is nonzero.
        public static byte StoryProgress51A6 { get; private set; }

        // DAT_1048_46B4 / DAT_1048_51AA ending bridge.
        // Hamerstein state-09 clears 46B4 and sets 51AA; the outer original
        // dispatcher consumes 51AA and transitions into ending.fli playback.
        public static byte GameplayState46B4 { get; private set; }
        public static bool EndingRequested51AA { get; private set; }

        // FUN_1010_8C0A player-damage/death globals.
        // 4C0E is set to 3 whenever computed GUARD damage is nonzero.
        // 46AC is latched to 1 on lethal player damage.
        // 4C1A receives attacking GUARD +0x08, i.e. the OBJECT slot index.
        public static ushort GuardDamageMarker4C0E { get; private set; }
        public static bool PlayerDeathLatch46AC { get; private set; }
        public static ushort PlayerDeathSource4C1A { get; private set; }

        // DAT_1048_4BE5. When nonzero FUN_8C0A still computes/marks damage
        // but suppresses all player-health/death commits. Other original paths
        // also replenish HP/ammo while this protection flag is active.
        public static bool PlayerDamageSuppressed4BE5 { get; set; }

        // DAT_1048_53F8: per-frame angular step in degrees, produced by D7D0.
        public static ushort FrameTurnStepDegrees53F8 { get; private set; } = 5;

        // Compatibility surface kept for callers written before the 51A6 closure.
        public static bool GuardAttackClass16FullDamageOverride
        {
            get => StoryProgress51A6 != 0;
            set => StoryProgress51A6 = value ? (byte)1 : (byte)0;
        }

        /// <summary>
        /// Confirmed DAT_1048_4C2E producer from FUN_1010_BFD8.
        /// In episode 1, level 9, semantic wall class 0x47 jams player firing
        /// and class 0x48 clears the jam. The original emits event 0x44 only
        /// when the latch actually changes.
        /// </summary>
        public static bool TryLoadExactTrigQ10(
            string path = "data/N3D_TRIG_Q10.BIN")
        {
            if (System.IO.File.Exists(path))
            {
                // OriginalTrigQ10.Load validates both byte length and the known
                // cardinal/45-degree values. If a file exists but is invalid,
                // propagate the error rather than silently using wrong projection.
                ExactTrigQ10 =
                    OriginalTrigQ10.Load(path);
                return true;
            }

            // Convenience path for a clean-room development checkout: when the
            // extracted BIN is absent, read the exact tables directly from the
            // audited Win16 1.10 executable. Hash validation is mandatory.
            string[] exeCandidates =
            {
                "data/NITE3W.EXE",
                "NITE3W.EXE",
                "data/NITE3W-10.EXE",
                "NITE3W-10.EXE"
            };

            foreach (string exePath in exeCandidates)
            {
                if (!System.IO.File.Exists(exePath))
                    continue;

                ExactTrigQ10 =
                    OriginalTrigQ10.LoadFromNite3w110Exe(
                        exePath);
                return true;
            }

            ExactTrigQ10 = null;
            return false;
        }

        /// <summary>
        /// Ordinary-positive-duration model of Win16 FUN_1010_D7D0:
        /// raw mean is stored before the local 40-ms minimum is applied.
        /// D=max(raw,40); 53F4=(1000+D/2)/D; 53F6=max(1,(D+2)/4);
        /// 53F8=max(1,(360*D+1400)/2800); 53FA=2*53F6.
        /// </summary>
        public static void ConfigureShadePalette(
            byte[] palette)
        {
            ShadeRuntime.Rebuild(
                palette,
                ShadeRuntime.ShadeIndex);
        }

        public static void SetShadeIndex(
            int shadeIndex)
        {
            ShadeRuntime.Rebuild(
                GameWindow.pal,
                shadeIndex);
        }

        public static void ConfigureFrameCalibration(uint rawMeanMs)
        {
            RawFrameMeanMs = unchecked((ushort)rawMeanMs);

            uint effective = rawMeanMs < 40 ? 40u : rawMeanMs;

            uint frameRate =
                (1000u + effective / 2u) / effective;
            FrameRateParameter53F4 =
                (ushort)Math.Min(frameRate, ushort.MaxValue);

            uint movement =
                (effective + 2u) / 4u;
            if (movement < 1)
                movement = 1;
            MovementStep53F6 =
                (ushort)Math.Min(movement, ushort.MaxValue);

            uint turn =
                (360u * effective + 1400u) / 2800u;
            if (turn < 1)
                turn = 1;
            TurnStep53F8 =
                (ushort)Math.Min(turn, ushort.MaxValue);

            ProjectileSubstepsPerTick =
                Math.Min(
                    ushort.MaxValue,
                    MovementStep53F6 * 2);
        }

        public static bool ApplyWeaponJamScriptTouchBFD8(
            int episode,
            int levelNumber,
            byte wallClass)
        {
            if (episode != 1 || levelNumber != 9)
                return false;

            bool next;
            if (wallClass == 0x47)
                next = true;
            else if (wallClass == 0x48)
                next = false;
            else
                return false;

            if (WeaponRuntime.Jammed == next)
                return false;

            WeaponRuntime.Jammed = next;
            SoundEffect.PlayOriginalEvent(0x44);
            return true;
        }

        public static bool ApplyScriptTouchProgress51A6(
            int episode,
            int levelNumber,
            byte wallClass)
        {
            if (wallClass != 0x47 || StoryProgress51A6 != 0)
                return false;

            bool activates =
                (episode == 1 && (levelNumber == 7 || levelNumber == 10)) ||
                (episode == 2 && levelNumber == 10) ||
                (episode == 3 && (levelNumber == 1 || levelNumber == 10));

            if (!activates)
                return false;

            StoryProgress51A6 = 1;
            return true;
        }

        public static bool AdvanceScriptProgress51A6ToPhase2()
        {
            if (StoryProgress51A6 != 1)
                return false;

            StoryProgress51A6 = 2;
            return true;
        }

        public static void BeginFrame(float deltaSeconds)
        {
            SlowLogicTickDue = false;
            GuardLogicTickDue = false;
            ProjectileLogicTickDue = false;

            if (deltaSeconds <= 0)
                return;

            // D7D0 clamps effective frame duration to at least 40 ms and derives
            // the angular step at DS:53F8 as:
            //   max(1, (frameMs*360 + 1400) / 2800)
            // Keep this value available to the recovered player-death camera.
            int effectiveFrameMs =
                Math.Max(40, (int)Math.Round(deltaSeconds * 1000.0));
            FrameTurnStepDegrees53F8 =
                (ushort)Math.Max(
                    1,
                    (effectiveFrameMs * 360 + 1400) / 2800);

            // Maintain the 32-bit absolute runtime clock used by OBJECT/projectile
            // animation deadlines. Preserve fractional milliseconds so a fast host
            // does not accumulate per-frame rounding drift.
            runtimeClockFractionMs += deltaSeconds * 1000.0;
            uint wholeMilliseconds = (uint)Math.Floor(runtimeClockFractionMs);
            if (wholeMilliseconds != 0)
            {
                RuntimeClockMs = unchecked(RuntimeClockMs + wholeMilliseconds);
                runtimeClockFractionMs -= wholeMilliseconds;
            }

            // The calibrated Win16 frame path clamps its effective duration to
            // at least 40 ms. On a modern host this yields the 25-Hz baseline
            // projectile update with 53FA=20 one-world-unit DDA substeps.
            projectileLogicAccumulator += deltaSeconds;
            if (projectileLogicAccumulator >= ProjectileLogicTickSeconds)
            {
                ProjectileLogicTickDue = true;
                // Like the original bucket schedulers, do not replay every
                // missed tick in one frame; preserve only phase.
                projectileLogicAccumulator %= ProjectileLogicTickSeconds;
            }

            guardLogicAccumulator += deltaSeconds;
            if (guardLogicAccumulator < GuardLogicTickSeconds)
                return;

            SlowLogicTickDue = true;
            GuardLogicTickDue = AutonomousGuardRuntimeEnabled;

            // D974 increments the weapon cadence counter on the same recovered
            // 8-Hz slow update used by GUARD state processing.
            WeaponRuntime.AdvanceSlowTick();

            // D70A observes the current 125-ms bin rather than replaying every
            // skipped bin. Preserve phase but drop catch-up iterations.
            guardLogicAccumulator %= GuardLogicTickSeconds;
        }

        public static void SetPlayerAreaSelector(byte areaId)
        {
            PlayerAreaSelector = areaId;
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
            guardLogicAccumulator = 0;
            projectileLogicAccumulator = 0;
            runtimeClockFractionMs = 0;
            RuntimeClockMs = 0;
            ProjectileDefinitionsReady = false;
            projectilePlasmaFlightDefinition = 0;
            projectilePlasmaImpactDefinition = 0;
            projectileMagicFlightDefinition = 0;
            projectileMagicImpactDefinition = 0;
            SlowLogicTickDue = false;
            GuardLogicTickDue = false;
            ProjectileLogicTickDue = false;
            PlayerInvisible = false;
            CurrentRenderGeneration = 0;
            GameplayState46B4 = 0;
            EndingRequested51AA = false;
            GuardDamageMarker4C0E = 0;
            PlayerDeathLatch46AC = false;
            PlayerDeathSource4C1A = 0;
            PlayerDamageSuppressed4BE5 = false;
            FrameTurnStepDegrees53F8 = 5;
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

            if (hasMapSemantics)
            {
                obj.MapCellBinding = OriginalMapTables.CellBindingHandle(
                    obj.WorldX >> 6,
                    obj.WorldY >> 6);
            }

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

            // FUN_6258 runs after GUARD construction: derive +0x0F from
            // SEQDEF bytes +6/+8 and force the initial directional sequence.
            if (ObjectDefinitions.TryGetHeader(
                    obj.DefinitionId,
                    out var directionDefinition))
            {
                guard.Control =
                    OriginalGuardProfiles.DirectionalControlMode(
                        directionDefinition);

                if (Game.player != null)
                {
                    OriginalGuardDispatcher.RefreshDirectionalSequence(
                        ref guard,
                        ref obj,
                        directionDefinition,
                        ToWorldCoordinate(Game.player.position.X),
                        ToWorldCoordinate(Game.player.position.Y),
                        true);
                }
            }

            bindings[entity] = new Binding
            {
                ObjectSlot = objectSlot,
                GuardSlot = guardSlot
            };

            return true;
        }

        public static bool RegisterMapGuard(
            Entity entity,
            byte mapObjectId)
        {
            if (entity == null ||
                mapObjectId == 0 ||
                Level.originalMap == null)
            {
                return false;
            }

            byte propertyFlags =
                Level.originalMap.ObjectProperty[
                    mapObjectId];

            if ((propertyFlags &
                    OriginalMapTables.ObjectCreatesGuard) == 0)
            {
                return false;
            }

            // Map semantics were validated above, so RegisterGuard cannot reach
            // its legacy GuardType fallback. The enum value is intentionally
            // irrelevant on this path.
            return RegisterGuard(
                entity,
                default(GuardType),
                mapObjectId);
        }

        public static bool RegisterWorldObject(
            Entity entity,
            byte mapObjectId)
        {
            if (entity == null ||
                mapObjectId == 0 ||
                Level.originalMap == null)
            {
                return false;
            }

            if (bindings.TryGetValue(
                    entity,
                    out var existing))
            {
                return existing.ObjectSlot >= 0;
            }

            if (!Level.originalMap.TryGetObjectClassAndVariant(
                    mapObjectId,
                    out byte objectClass,
                    out byte variant))
            {
                return false;
            }

            byte propertyFlags =
                Level.originalMap.ObjectProperty[
                    mapObjectId];

            if ((propertyFlags &
                    OriginalMapTables.ObjectRuntimePresent) == 0)
            {
                return false;
            }

            if (!RegisterMapObjectDefinition(
                    mapObjectId,
                    out byte definitionId))
            {
                return false;
            }

            if (ObjectCount >= Objects.Length)
            {
                throw new InvalidOperationException(
                    "Original OBJECT pool capacity exceeded.");
            }

            int objectSlot =
                ObjectCount++;

            ref var obj =
                ref Objects[objectSlot];

            short worldX =
                ToWorldCoordinate(
                    entity.position.X);
            short worldY =
                ToWorldCoordinate(
                    entity.position.Y);

            OriginalWorldObjectRuntime.InitializeMapObject(
                ref obj,
                mapObjectId,
                variant,
                propertyFlags,
                objectClass,
                definitionId,
                worldX,
                worldY);

            RefreshKnownWorldObjectVerticalAnchor(
                ref obj);

            bindings[entity] =
                new Binding
                {
                    ObjectSlot = objectSlot,
                    GuardSlot = -1
                };

            return true;
        }

        static void RefreshKnownWorldObjectVerticalAnchor(
            ref OriginalObjectRecord obj)
        {
            if (Img.current == null)
                return;

            int frameIndex =
                unchecked((byte)obj.Component03);

            if (!ObjectDefinitions.TryGetBitmapFrame(
                    obj.DefinitionId,
                    frameIndex,
                    Img.current.rawData,
                    out BitmapImage frame))
            {
                return;
            }

            OriginalWorldObjectRuntime.UpdateKnownVerticalAnchor(
                ref obj,
                frame.height);
        }

        public static bool TickBoundWorldObjectPresentation(
            Entity entity)
        {
            if (!bindings.TryGetValue(
                    entity,
                    out var binding) ||
                binding.ObjectSlot < 0 ||
                binding.GuardSlot >= 0)
            {
                return false;
            }

            ref var obj =
                ref Objects[binding.ObjectSlot];

            if ((obj.Flags &
                    OriginalMapTables.ObjectRuntimePresent) == 0 ||
                !ObjectDefinitions.TryGetHeader(
                    obj.DefinitionId,
                    out var definition))
            {
                return false;
            }

            bool advanced;

            if ((obj.ObjectClass == 0x2C ||
                 obj.ObjectClass == 0x2D) &&
                Game.player != null)
            {
                advanced =
                    OriginalWorldObjectRuntime
                        .AdvanceDirectionalAnimation(
                            ref obj,
                            definition,
                            RuntimeClockMs,
                            ToWorldCoordinate(
                                Game.player.position.X),
                            ToWorldCoordinate(
                                Game.player.position.Y));
            }
            else
            {
                advanced =
                    OriginalWorldObjectRuntime
                        .AdvancePresentationAnimationIfDue(
                            ref obj,
                            definition,
                            RuntimeClockMs,
                            OriginalRandom.Next);
            }

            if (advanced)
            {
                RefreshKnownWorldObjectVerticalAnchor(
                    ref obj);
            }

            return advanced;
        }

        public static bool DeactivateCollectedWorldObject(
            Entity entity)
        {
            if (!bindings.TryGetValue(
                    entity,
                    out var binding) ||
                binding.ObjectSlot < 0 ||
                binding.GuardSlot >= 0)
            {
                return false;
            }

            ref var obj =
                ref Objects[binding.ObjectSlot];

            if (Level.originalMap != null)
            {
                int tileX =
                    obj.WorldX >> 6;
                int tileY =
                    obj.WorldY >> 6;

                if (tileX >= 0 &&
                    tileY >= 0 &&
                    tileX < OriginalRuntime.MapWidth &&
                    tileY < OriginalRuntime.MapHeight)
                {
                    Level.originalMap.ObjectId[
                        tileX,
                        tileY] = 0;
                }
            }

            OriginalWorldObjectRuntime.DeactivateCollected(
                ref obj);

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

        public static bool RegisterProjectileDefinitions()
        {
            if (Img.current == null)
                return false;

            if (!RegisterMapObjectDefinition(
                    (byte)ObjectType.Missileflyingplasmabolt,
                    out byte plasmaFlight) ||
                !RegisterMapObjectDefinition(
                    (byte)ObjectType.Missileexplodingplasmabolt,
                    out byte plasmaImpact) ||
                !RegisterMapObjectDefinition(
                    (byte)ObjectType.Missileflyingspellstars,
                    out byte magicFlight) ||
                !RegisterMapObjectDefinition(
                    (byte)ObjectType.Missileexplodingspellstars,
                    out byte magicImpact))
            {
                ProjectileDefinitionsReady = false;
                return false;
            }

            projectilePlasmaFlightDefinition = plasmaFlight;
            projectilePlasmaImpactDefinition = plasmaImpact;
            projectileMagicFlightDefinition = magicFlight;
            projectileMagicImpactDefinition = magicImpact;
            ProjectileDefinitionsReady = true;
            return true;
        }

        public static bool TryGetProjectileDefinition(
            OriginalWeaponSelector weaponSelector,
            bool impact,
            out byte definitionId,
            out OriginalObjectDefinitionRecord definition)
        {
            definitionId = 0;
            definition = default;

            if (!ProjectileDefinitionsReady)
                return false;

            switch (weaponSelector)
            {
                case OriginalWeaponSelector.SingleShotLaser:
                case OriginalWeaponSelector.ContinuousLaser:
                    definitionId = impact
                        ? projectilePlasmaImpactDefinition
                        : projectilePlasmaFlightDefinition;
                    break;

                case OriginalWeaponSelector.MagicWand:
                    definitionId = impact
                        ? projectileMagicImpactDefinition
                        : projectileMagicFlightDefinition;
                    break;

                default:
                    return false;
            }

            return ObjectDefinitions.TryGetHeader(
                definitionId,
                out definition);
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

            int eventId =
                OriginalGuardSounds.AlertSoundId(objectClass, randomValue);
            SoundEffect.PlayOriginalEvent(eventId);
        }

        static void ConsumeOriginalAttackSoundSelection(byte objectClass)
        {
            ushort randomValue = OriginalGuardSounds.AttackUsesRandom(objectClass)
                ? OriginalRandom.Next()
                : (ushort)0;

            int eventId =
                OriginalGuardSounds.AttackSoundId(objectClass, randomValue);
            SoundEffect.PlayOriginalEvent(eventId);
        }

        static void PlayOriginalDeathSound(byte objectClass)
        {
            ushort randomValue = OriginalGuardSounds.DeathUsesRandom(objectClass)
                ? OriginalRandom.Next()
                : (ushort)0;

            int eventId =
                OriginalGuardSounds.DeathSoundId(objectClass, randomValue);
            SoundEffect.PlayOriginalEvent(eventId);
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
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj)
        {
            if (Game.player == null)
                return false;

            // FUN_8C0A does not apply further health changes once the player is
            // already in gameplay state 2 (death).
            if (GameplayState46B4 == 2)
                return true;

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

            int amount = damage.DifficultyTransformed;
            if (amount != 0)
                GuardDamageMarker4C0E = 3;

            if (PlayerDamageSuppressed4BE5)
                return false;

            int hp = Game.player.health;
            if (amount >= hp)
            {
                Game.player.health = 0;
                GameplayState46B4 = 2;
                PlayerDeathLatch46AC = true;
                PlayerDeathSource4C1A =
                    guard.ObjectSlot;

                // FUN_80EA: the attacking GUARD enters the shared no-local-action
                // state immediately when its hit kills the player.
                guard.State =
                    (byte)OriginalGuardState.LethalPlayerContact0B;

                // FUN_8C0A emits original event 10 with the long 0x20000 timing
                // argument. The current event bridge owns host playback timing.
                SoundEffect.PlayOriginalEvent(10);
                return true;
            }

            Game.player.health = hp - amount;
            return false;
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

            // FUN_7E54 is shared by 0C/0D and calls FUN_6EE0 with
            // param5=0. It refreshes only when the result octant changes and
            // never leaves state 0C/0D.
            return OriginalGuardDispatcher.RefreshDirectionalSequence(
                ref guard,
                ref obj,
                definition,
                playerWorldX,
                playerWorldY,
                false);
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
            // FUN_AE56(1) emits original SND event 0x45 before restoring the
            // normal level music through C63E. Game.Update already owns the
            // normal level-song selection, so only the recovered event is
            // required here.
            SoundEffect.PlayOriginalEvent(0x45);

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

            // C63E restores normal level music in the original; the current
            // Game scene already keeps that level song selected continuously.
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

                        if (ApplyGuardAttackDamageToPlayer(ref guard, ref obj))
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
                    PlayOriginalDeathSound(obj.ObjectClass);
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
                        RemoteCannonsEnabled);
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
                        ApplyGuardAttackDamageToPlayer(ref guard, ref obj);

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
                    if (Game.player == null || Level.originalMap == null)
                        return OriginalGuardDispatchResult.NotHandled;

                    int sourceTileX = obj.WorldX >> 6;
                    int sourceTileY = obj.WorldY >> 6;

                    short playerWorldX =
                        ToWorldCoordinate(Game.player.position.X);
                    short playerWorldY =
                        ToWorldCoordinate(Game.player.position.Y);
                    int playerTileX = playerWorldX >> 6;
                    int playerTileY = playerWorldY >> 6;

                    byte triggerOctant = guard.Octant;

                    var result = OriginalGuardDispatcher.TickState13(
                        ref guard,
                        ref obj,
                        (candidateX, candidateY) =>
                        {
                            int targetTileX = candidateX >> 6;
                            int targetTileY = candidateY >> 6;

                            return Level.TryCommitOriginalState13MapMove(
                                sourceTileX,
                                sourceTileY,
                                targetTileX,
                                targetTileY,
                                playerTileX,
                                playerTileY);
                        },
                        () =>
                        {
                            Level.TriggerOriginalState13OneShot(
                                sourceTileX,
                                sourceTileY,
                                triggerOctant);
                        });

                    if (result == OriginalGuardDispatchResult.Moved)
                    {
                        obj.MapCellBinding = OriginalMapTables.CellBindingHandle(
                            obj.WorldX >> 6,
                            obj.WorldY >> 6);

                        entity.position.X =
                            (float)obj.WorldX / OriginalRuntime.WorldUnitsPerTile;
                        entity.position.Y =
                            (float)obj.WorldY / OriginalRuntime.WorldUnitsPerTile;

                        SyncGuardPosition(entity);
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

        public static bool RecordObjectProjection(
            Entity entity,
            short projectedBaseRow)
        {
            if (!bindings.TryGetValue(
                    entity,
                    out var binding) ||
                binding.ObjectSlot < 0)
            {
                return false;
            }

            // OBJECT+0x18 is persistent: projection overwrites it only after a
            // successful visible sprite projection; failed/off-screen frames do
            // not clear the previous cached row.
            Objects[binding.ObjectSlot].ProjectedBaseRow =
                projectedBaseRow;

            return true;
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

            RecordObjectProjection(
                entity,
                projectedBaseRow);

            ref var guard =
                ref Guards[binding.GuardSlot];

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

        static bool HitscanStateEligible(ref OriginalGuardRecord guard)
        {
            return guard.Strength != 0 &&
                   guard.State != (byte)OriginalGuardState.AnimationTimer &&
                   guard.State != (byte)OriginalGuardState.DeathFinalize09 &&
                   guard.State != (byte)OriginalGuardState.NoLocalAction0A;
        }

        public static GuardHitResult ApplyPlayerWeaponDamage(
            Entity entity,
            OriginalWeaponSelector weaponSelector,
            bool requireCurrentAimStamp)
        {
            if (!bindings.TryGetValue(entity, out var binding) ||
                binding.GuardSlot < 0 ||
                binding.ObjectSlot < 0)
            {
                return GuardHitResult.NoRuntimeBinding;
            }

            ref var guard = ref Guards[binding.GuardSlot];
            ref var obj = ref Objects[binding.ObjectSlot];

            if (guard.Strength == 0)
                return GuardHitResult.NoDamage;

            if (requireCurrentAimStamp &&
                (CurrentRenderGeneration == 0 ||
                 guard.Timestamp != CurrentRenderGeneration ||
                 !HitscanStateEligible(ref guard)))
            {
                return GuardHitResult.NoDamage;
            }

            var computed = OriginalDamage.ComputePlayerToGuard(
                obj.ProjectedBaseRow,
                OriginalRuntime.ViewportCenterY,
                obj.ObjectClass,
                (byte)weaponSelector,
                Difficulty,
                (byte)Game.episode,
                OriginalRandom.Next());

            bool perceptionSucceeded =
                EvaluateGuardPerception(
                    entity,
                    false,
                    false);

            return ApplyNormalGuardDamage(
                entity,
                computed.StoredByte,
                OriginalRandom.Next,
                perceptionSucceeded);
        }

        public static int FireHitscan(OriginalWeaponSelector weaponSelector)
        {
            if (weaponSelector != OriginalWeaponSelector.SilverPistol ||
                Game.player == null)
            {
                return 0;
            }

            short playerWorldX = ToWorldCoordinate(Game.player.position.X);
            short playerWorldY = ToWorldCoordinate(Game.player.position.Y);
            int playerTileX = playerWorldX >> 6;
            int playerTileY = playerWorldY >> 6;
            int hits = 0;

            foreach (var pair in bindings)
            {
                var binding = pair.Value;
                if (binding.GuardSlot < 0 ||
                    binding.ObjectSlot < 0)
                {
                    continue;
                }

                ref var guard = ref Guards[binding.GuardSlot];
                ref var obj = ref Objects[binding.ObjectSlot];

                if (!HitscanStateEligible(ref guard) ||
                    CurrentRenderGeneration == 0 ||
                    guard.Timestamp != CurrentRenderGeneration)
                {
                    continue;
                }

                int guardTileX = obj.WorldX >> 6;
                int guardTileY = obj.WorldY >> 6;

                if (!Level.OriginalPerceptionLineTrace(
                        playerTileX,
                        playerTileY,
                        guardTileX - playerTileX,
                        guardTileY - playerTileY,
                        16,
                        true))
                {
                    continue;
                }

                var result = ApplyPlayerWeaponDamage(
                    pair.Key,
                    weaponSelector,
                    true);

                if (result != GuardHitResult.NoRuntimeBinding &&
                    result != GuardHitResult.NoDamage &&
                    result != GuardHitResult.MissingDefinition)
                {
                    hits++;
                }
            }

            return hits;
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

        public static void ResetPlayerScoreNewGame()
        {
            PlayerScore = 0;
        }

        static int CommitGuardKillScore(byte objectClass)
        {
            int delta = OriginalRuntime.GuardScore(objectClass);
            unchecked
            {
                PlayerScore += delta;
            }
            return delta;
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
                // FUN_1010_80F8 commits score on lethal defeat before the
                // death-animation/state-09 finalizer. Strength was nonzero on
                // entry, so this path can award the class score only once.
                CommitGuardKillScore(obj.ObjectClass);

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
                    // FUN_A0EE Hamerstein terminal hook:
                    // event 0x12, gameplay state 46B4=0, ending latch 51AA=1.
                    // The outer dispatcher later consumes 51AA and starts
                    // ending.fli; this runtime bridge exposes that latch while
                    // presentation remains owned by the scene layer.
                    SoundEffect.PlayOriginalEvent(0x12);
                    GameplayState46B4 = 0;
                    EndingRequested51AA = true;
                    return GuardHitResult.Killed;

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
