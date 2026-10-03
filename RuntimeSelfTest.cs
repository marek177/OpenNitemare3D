using System;
using System.IO;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Nitemare3D
{
    public static class RuntimeSelfTest
    {
        static void Assert(bool condition, string message)
        {
            if (!condition)
                throw new InvalidOperationException("SELFTEST: " + message);
        }

        public static void Run()
        {
            TestRecordSizes();
            TestFrameCalibration();
            TestExactSpriteProjection();
            TestProjectedSpriteQueue();
            TestGuardLogicClock();
            TestWeaponRuntime();
            TestPickupRuntime();
            TestAutonomousGuardStateCoverage();
            TestObjectDefinitionCatalog();
            TestImgResourceLoader();
            TestOriginalMapTables();
            TestOriginalWallRuntime();
            TestExplodingWallRuntime();
            TestOriginalRandom();
            TestDamageMatrix();
            TestScriptProgress51A6();
            TestWeaponJamScriptBFD8();
            TestGuardSounds();
            TestGuardToPlayerDamage();
            TestPackedGuardSequences();
            TestExactAnimationSchedulers();
            TestHitTransitionRouting();
            TestAttackGate();
            TestPerceptionPrefilter();
            TestGuardGridTrace();
            TestGuardLosCellFlags();
            TestCellFlagGenerators();
            TestOriginalSoundEventMapping();
            TestProjectileRuntime();
            TestDelayState();
            TestState13Movement();
            TestState11Movement();
            TestMovementPlanning();
            TestMovementCell700A();
            TestMovementCollisionCore();
            TestDirectionalSequenceRefresh();
            TestGuardMapClassMapping();
            TestGuardInitialProfiles();
            TestState07Decision();
            TestState08WallTurn();
            TestStates0E0F10();
            TestState14DanceCycle();
            TestPainReturn();
            TestDoorSelector();
            TestWakeCache();

            Console.WriteLine("OpenNitemare3D runtime self-test: PASS");
        }

        static void TestRecordSizes()
        {
            Assert(Marshal.SizeOf<OriginalObjectRecord>() == 28,
                "OBJECT record must be 28 bytes.");
            Assert(Marshal.SizeOf<OriginalGuardRecord>() == 26,
                "GUARD record must be 26 bytes.");
            Assert(Marshal.SizeOf<OriginalDoorRuntimeRecord>() == 22,
                "DOOR record must be 22 bytes.");
            Assert(Marshal.OffsetOf<OriginalDoorRuntimeRecord>(
                       nameof(OriginalDoorRuntimeRecord.State)).ToInt32() == 0x0C &&
                   Marshal.OffsetOf<OriginalDoorRuntimeRecord>(
                       nameof(OriginalDoorRuntimeRecord.Timer)).ToInt32() == 0x0E &&
                   Marshal.OffsetOf<OriginalDoorRuntimeRecord>(
                       nameof(OriginalDoorRuntimeRecord.TargetX)).ToInt32() == 0x10 &&
                   Marshal.OffsetOf<OriginalDoorRuntimeRecord>(
                       nameof(OriginalDoorRuntimeRecord.TargetY)).ToInt32() == 0x12 &&
                   Marshal.OffsetOf<OriginalDoorRuntimeRecord>(
                       nameof(OriginalDoorRuntimeRecord.Latch)).ToInt32() == 0x14,
                "DOOR record field offsets must match Win16 0x16-byte layout.");
            Assert(Marshal.SizeOf<OriginalProjectileRecord>() == 42,
                "projectile record must be 42 bytes.");
        }

        static void TestFrameCalibration()
        {
            OriginalRuntimeState.ConfigureFrameCalibration(10);
            Assert(OriginalRuntimeState.RawFrameMeanMs == 10 &&
                   OriginalRuntimeState.FrameRateParameter53F4 == 25 &&
                   OriginalRuntimeState.MovementStep53F6 == 10 &&
                   OriginalRuntimeState.TurnStep53F8 == 5 &&
                   OriginalRuntimeState.ProjectileSubstepsPerTick == 20,
                "D7D0 minimum-40-ms calibration mismatch.");

            OriginalRuntimeState.ConfigureFrameCalibration(80);
            Assert(OriginalRuntimeState.RawFrameMeanMs == 80 &&
                   OriginalRuntimeState.FrameRateParameter53F4 == 13 &&
                   OriginalRuntimeState.MovementStep53F6 == 20 &&
                   OriginalRuntimeState.TurnStep53F8 == 10 &&
                   OriginalRuntimeState.ProjectileSubstepsPerTick == 40,
                "D7D0 80-ms calibration mismatch.");

            // Restore the portable/default effective 40-ms profile for all
            // subsequent runtime tests.
            OriginalRuntimeState.ConfigureFrameCalibration(40);
        }

        static void TestExactSpriteProjection()
        {
            string path = Path.GetTempFileName();

            try
            {
                byte[] table =
                    new byte[OriginalTrigQ10.FileSize];

                void WriteInt16(int offset, short value)
                {
                    ushort raw = unchecked((ushort)value);
                    table[offset + 0] = (byte)raw;
                    table[offset + 1] = (byte)(raw >> 8);
                }

                // Minimal exact-table fixture covering loader sanity anchors and
                // the cardinal projections exercised below.
                WriteInt16(0 * 2, 0);
                WriteInt16(45 * 2, 724);
                WriteInt16(90 * 2, 1024);
                WriteInt16(180 * 2, 0);
                WriteInt16(270 * 2, -1024);

                int cosBase = 720;
                WriteInt16(cosBase + 0 * 2, 1024);
                WriteInt16(cosBase + 45 * 2, 724);
                WriteInt16(cosBase + 90 * 2, 0);
                WriteInt16(cosBase + 180 * 2, -1024);
                WriteInt16(cosBase + 270 * 2, 0);

                File.WriteAllBytes(path, table);

                OriginalTrigQ10 trig =
                    OriginalTrigQ10.Load(path);

                Assert(OriginalProjectionExact.ProjectPoint(
                           0,
                           0,
                           64,
                           0,
                           trig,
                           out var point) &&
                       point.ScreenX == OriginalRendererCore.CenterX &&
                       point.ProjectedYQ4 == 2526 &&
                       point.DepthQ10 == 65536,
                    "E5D8 north/cardinal point projection mismatch.");

                var vec = new OriginalRendererCore.Vec
                {
                    X1 = 68,
                    Y1 = 36,
                    X2 = 132,
                    Y2 = 36
                };

                Assert(OriginalProjectionExact.ProjectVec(
                           vec,
                           100,
                           100,
                           0,
                           trig) &&
                       vec.ScreenX1 == 71 &&
                       vec.ScreenX2 == 249 &&
                       vec.ProjectedY1Q4 == 2526 &&
                       vec.ProjectedY2Q4 == 2526,
                    "E798 cardinal VEC projection coefficient order mismatch.");

                var frame = new BitmapImage
                {
                    width = 32,
                    height = 32,
                    data = new byte[32, 32]
                };

                var obj = new OriginalObjectRecord
                {
                    Flags = 0x01,
                    WorldX = 100,
                    WorldY = 36,
                    Runtime1A = 5
                };

                Assert(OriginalSpriteProjectionExact.TryProject(
                           obj,
                           frame,
                           100,
                           100,
                           0,
                           trig,
                           out var projected) &&
                       projected.BaselineRow == 157 &&
                       projected.Height == 77 &&
                       projected.Width == 77 &&
                       projected.Left == 122 &&
                       projected.Right == 199 &&
                       projected.Top == 67 &&
                       projected.Bottom == 144,
                    "CC7C 32x32 object-sprite projection mismatch.");

                obj.WorldX = 164;
                obj.WorldY = 100;

                Assert(OriginalSpriteProjectionExact.TryProject(
                           obj,
                           frame,
                           100,
                           100,
                           90,
                           trig,
                           out var eastProjected) &&
                       eastProjected.BaselineRow == projected.BaselineRow &&
                       eastProjected.Left == projected.Left &&
                       eastProjected.Right == projected.Right,
                    "E5D8 east/cardinal projection must match equal-depth north geometry.");
            }
            finally
            {
                File.Delete(path);
            }
        }

        static void TestProjectedSpriteQueue()
        {
            Assert(
                OriginalProjectedSpriteQueue
                    .WallVisibilityQ4FromPerpendicularDistance(1.0) ==
                2526,
                "one-tile wall visibility Q4 mismatch.");

            Assert(
                OriginalProjectedSpriteQueue
                    .WallVisibilityQ4FromPerpendicularDistance(2.0) ==
                1903,
                "two-tile wall visibility Q4 mismatch.");

            ushort[] visibility =
                new ushort[OriginalRendererCore.ScreenWidth];

            visibility[120] = 2600;
            visibility[160] = 2500;
            visibility[200] = 2700;

            Assert(OriginalProjectedSpriteQueue.PassesThreeColumnWallGate(
                       visibility,
                       120,
                       160,
                       200,
                       2526),
                "CC7C three-column gate must pass when any probe wall is behind the sprite.");

            Assert(!OriginalProjectedSpriteQueue.PassesThreeColumnWallGate(
                       visibility,
                       120,
                       160,
                       200,
                       2400),
                "CC7C three-column gate must reject when all probe walls are in front.");

            Assert(OriginalProjectedSpriteQueue.ColumnPassesWall(
                       visibility,
                       120,
                       2400,
                       true),
                "OBJECT flag 0x10 wall-bypass path must ignore per-column wall depth.");

            uint step =
                OriginalProjectedSpriteQueue.SpriteSourceStep16_16(
                    32,
                    67,
                    144);

            Assert(step == 26886 &&
                   OriginalProjectedSpriteQueue.SpriteSourceCoordinate(
                       122,
                       122,
                       step) == 0 &&
                   OriginalProjectedSpriteQueue.SpriteSourceCoordinate(
                       199,
                       122,
                       step) == 31,
                "3EDC/36C8 16.16 sprite source sampling mismatch.");

            bool[] occupied =
                new bool[OriginalProjectedSpriteQueue.SlotCount];

            Assert(OriginalProjectedSpriteQueue.FindFreeSlot(
                       occupied,
                       100) == 20,
                "sprite queue must prefer baseline-center slot.");

            occupied[20] = true;
            occupied[19] = true;
            Assert(OriginalProjectedSpriteQueue.FindFreeSlot(
                       occupied,
                       100) == 18,
                "sprite queue first pass must walk toward the horizon.");

            for (int i = 0; i <= 74; i++)
                occupied[i] = true;

            Assert(OriginalProjectedSpriteQueue.FindFreeSlot(
                       occupied,
                       155) == 75,
                "sprite queue second pass must reach the viewport-bottom slot.");

            occupied[75] = true;
            Assert(OriginalProjectedSpriteQueue.FindFreeSlot(
                       occupied,
                       155) == -1,
                "normal viewport queue must not spill into physical slots 76..99.");
        }

        static void TestGuardLogicClock()
        {
            bool previous = OriginalRuntimeState.AutonomousGuardRuntimeEnabled;
            OriginalRuntimeState.AutonomousGuardRuntimeEnabled = true;
            OriginalRuntimeState.Reset();

            OriginalRuntimeState.BeginFrame(0.100f);
            Assert(!OriginalRuntimeState.GuardLogicTickDue,
                "GUARD clock must not tick before 125 ms.");

            OriginalRuntimeState.BeginFrame(0.024f);
            Assert(!OriginalRuntimeState.GuardLogicTickDue,
                "GUARD clock must still wait at 124 ms.");

            OriginalRuntimeState.BeginFrame(0.001f);
            Assert(OriginalRuntimeState.GuardLogicTickDue,
                "GUARD clock must tick at 125 ms.");

            OriginalRuntimeState.BeginFrame(0.001f);
            Assert(!OriginalRuntimeState.GuardLogicTickDue,
                "GUARD clock must emit at most one pulse per frame.");

            // A long frame advances to the current time bin but does not replay
            // every skipped 125-ms interval as multiple same-frame ticks.
            OriginalRuntimeState.BeginFrame(0.400f);
            Assert(OriginalRuntimeState.GuardLogicTickDue,
                "long frame must produce one GUARD tick.");
            OriginalRuntimeState.BeginFrame(0.001f);
            Assert(!OriginalRuntimeState.GuardLogicTickDue,
                "long frame must drop catch-up GUARD ticks.");

            OriginalRuntimeState.AutonomousGuardRuntimeEnabled = previous;
            OriginalRuntimeState.Reset();
        }

        static void TestWeaponRuntime()
        {
            var weapon = new OriginalWeaponRuntime();
            weapon.ResetNewGame();

            Assert(weapon.ActiveSelector == OriginalWeaponSelector.None &&
                   weapon.OwnedWeaponsMask == 0 &&
                   weapon.PistolAmmo == 0 &&
                   weapon.PlasmaAmmo == 0 &&
                   weapon.WandAmmo == 0,
                "new-game weapon block must start empty.");

            Assert(OriginalWeaponRuntime.FireThreshold(
                       OriginalWeaponSelector.SingleShotLaser) == 2 &&
                   OriginalWeaponRuntime.FireThreshold(
                       OriginalWeaponSelector.MagicWand) == 1 &&
                   OriginalWeaponRuntime.FireThreshold(
                       OriginalWeaponSelector.SilverPistol) == 3 &&
                   OriginalWeaponRuntime.FireThreshold(
                       OriginalWeaponSelector.ContinuousLaser) == 1,
                "weapon cadence threshold table mismatch.");

            weapon.AdvanceSlowTick();
            Assert(!weapon.TryAcceptFireAttempt(
                       OriginalWeaponSelector.SingleShotLaser,
                       true,
                       true),
                "weapon 0 must not fire before two slow ticks.");

            weapon.AdvanceSlowTick();
            Assert(weapon.TryAcceptFireAttempt(
                       OriginalWeaponSelector.SingleShotLaser,
                       true,
                       true) &&
                   weapon.CadenceCounter == 0,
                "weapon 0 cadence acceptance/reset mismatch.");

            weapon.AdvanceSlowTick();
            Assert(!weapon.TryAcceptFireAttempt(
                       OriginalWeaponSelector.SingleShotLaser,
                       false,
                       true),
                "weapons 0-2 must require FIRE edge.");

            Assert(weapon.TryAcceptFireAttempt(
                       OriginalWeaponSelector.ContinuousLaser,
                       false,
                       true),
                "weapon 3 must accept held FIRE after its threshold.");

            weapon.GrantWeapon(OriginalWeaponSelector.SingleShotLaser);
            Assert(weapon.HasWeapon(
                       OriginalWeaponSelector.SingleShotLaser) &&
                   weapon.ActiveSelector ==
                       OriginalWeaponSelector.SingleShotLaser &&
                   weapon.PlasmaAmmo == 50,
                "weapon pickup must own/select weapon and initialize ammo to 50.");

            Assert(weapon.ConsumeAmmo(
                       OriginalWeaponSelector.SingleShotLaser) &&
                   weapon.PlasmaAmmo == 49,
                "weapon 0 plasma ammo decrement mismatch.");

            weapon.GrantWeapon(OriginalWeaponSelector.ContinuousLaser);
            Assert(weapon.PlasmaAmmo == 50,
                "weapon 3 pickup must share/reset the plasma pool to 50.");
            Assert(weapon.ConsumeAmmo(
                       OriginalWeaponSelector.ContinuousLaser) &&
                   weapon.PlasmaAmmo == 49,
                "weapon 3 must consume the shared plasma pool.");

            weapon.GrantWeapon(OriginalWeaponSelector.MagicWand);
            weapon.GrantWeapon(OriginalWeaponSelector.SilverPistol);
            Assert(weapon.WandAmmo == 50 &&
                   weapon.PistolAmmo == 50,
                "wand/pistol weapon-start ammo mismatch.");

            weapon.SetAmmo(OriginalWeaponSelector.SilverPistol, 99);
            Assert(weapon.AddAmmoPickup(
                       OriginalWeaponSelector.SilverPistol) &&
                   weapon.PistolAmmo == 119,
                "ammo pickup must preserve transient 99+20 before clamp.");
            weapon.NormalizeAmmoCaps();
            Assert(weapon.PistolAmmo == 100 &&
                   !weapon.AddAmmoPickup(
                       OriginalWeaponSelector.SilverPistol),
                "ammo normalization/cap rejection mismatch.");

            weapon.SetAmmo(OriginalWeaponSelector.MagicWand, 7);
            weapon.Omnipotent = true;
            Assert(weapon.ConsumeAmmo(OriginalWeaponSelector.MagicWand) &&
                   weapon.WandAmmo == 7,
                "Omnipotent must bypass ammo decrement.");

            weapon.Omnipotent = false;
            weapon.Jammed = true;
            Assert(!weapon.ConsumeAmmo(OriginalWeaponSelector.MagicWand) &&
                   weapon.WandAmmo == 7,
                "weapon jam must reject shot acceptance without ammo loss.");
        }

        static void TestPickupRuntime()
        {
            var pickups = new OriginalPickupRuntime();
            pickups.ResetNewGame();

            var player = new Player();
            var weapons = OriginalRuntimeState.WeaponRuntime;
            weapons.ResetNewGame();

            Assert(pickups.TryCollectDirectMapPickup(
                       0x2F, 2, player, weapons) &&
                   pickups.KeyMask == 0x04,
                "CF60 key variant must set DAT_4C28 bit.");

            Assert(pickups.TryCollectDirectMapPickup(
                       0x30, 1, player, weapons) &&
                   pickups.IdCardMask == 0x02,
                "CF60 ID-card variant must set DAT_4C29 bit.");

            player.health = 95;
            Assert(pickups.TryCollectDirectMapPickup(
                       0x33, 0, player, weapons) &&
                   player.health == 100,
                "full-strength potion must add 20 then clamp HP to 100.");
            Assert(!pickups.TryCollectDirectMapPickup(
                       0x33, 1, player, weapons),
                "health pickup at 100 must be rejected and remain in world.");

            player.health = 80;
            Assert(pickups.TryCollectDirectMapPickup(
                       0x33, 1, player, weapons) &&
                   player.health == 90,
                "half-strength potion must add 10 HP.");

            Assert(pickups.TryCollectDirectMapPickup(
                       0x36, 2, player, weapons) &&
                   weapons.HasWeapon(OriginalWeaponSelector.SilverPistol) &&
                   weapons.PistolAmmo == 50,
                "weapon variant 2 must grant Silver Pistol with 50 ammo.");

            weapons.SetAmmo(OriginalWeaponSelector.SilverPistol, 95);
            Assert(pickups.TryCollectDirectMapPickup(
                       0x39, 0, player, weapons) &&
                   weapons.PistolAmmo == 100,
                "ammo variant 0 must target pistol pool and clamp to 100.");
            Assert(!pickups.TryCollectDirectMapPickup(
                       0x39, 0, player, weapons),
                "full pistol ammo pool must reject pickup.");

            weapons.SetAmmo(OriginalWeaponSelector.SingleShotLaser, 10);
            Assert(pickups.TryCollectDirectMapPickup(
                       0x39, 1, player, weapons) &&
                   weapons.PlasmaAmmo == 30,
                "ammo variant 1 must target shared plasma pool.");

            weapons.SetAmmo(OriginalWeaponSelector.MagicWand, 10);
            Assert(pickups.TryCollectDirectMapPickup(
                       0x39, 2, player, weapons) &&
                   weapons.WandAmmo == 30,
                "ammo variant 2 must target wand pool.");

            for (int i = 0; i < 5; i++)
                Assert(pickups.TryCollectDirectMapPickup(
                           0x3A, 0, player, weapons),
                    "crystal-ball energy pickup should be accepted below 100.");
            Assert(pickups.EnemyLocatorEnergy == 100 &&
                   !pickups.TryCollectDirectMapPickup(
                       0x3A, 0, player, weapons),
                "enemy-locator energy must cap/reject at 100.");

            for (int i = 0; i < 5; i++)
                pickups.TryCollectDirectMapPickup(
                    0x3B, 0, player, weapons);
            Assert(pickups.MapClarityEnergy == 100,
                "map-clarity energy must cap at 100.");

            Assert(pickups.TryCollectDirectMapPickup(
                       0x3C, 3, player, weapons) &&
                   pickups.PentagramMask == 0x08,
                "pentagram variant must set DAT_4C45 bit.");

            Assert(OriginalPickupRuntime.PickupSoundEventId(0x2F) == 0x31 &&
                   OriginalPickupRuntime.PickupSoundEventId(0x33) == 0x2F &&
                   OriginalPickupRuntime.PickupSoundEventId(0x36) == 0x32 &&
                   OriginalPickupRuntime.PickupSoundEventId(0x39) == 0x34 &&
                   OriginalPickupRuntime.PickupSoundEventId(0x3A) == 0x2E &&
                   OriginalPickupRuntime.PickupSoundEventId(0x3B) == 0x33,
                "B7FC direct pickup sound-event mapping mismatch.");
        }

        static void TestAutonomousGuardStateCoverage()
        {
            OriginalGuardState[] confirmed =
            {
                OriginalGuardState.AnimationTimer,
                OriginalGuardState.Delay,
                OriginalGuardState.Active02,
                OriginalGuardState.Detection03,
                OriginalGuardState.DetectionAttack04,
                OriginalGuardState.Transition05,
                OriginalGuardState.MoveThen03,
                OriginalGuardState.Active07,
                OriginalGuardState.Move08,
                OriginalGuardState.DeathFinalize09,
                OriginalGuardState.NoLocalAction0A,
                OriginalGuardState.LethalPlayerContact0B,
                OriginalGuardState.Shared0C,
                OriginalGuardState.Shared0D,
                OriginalGuardState.Conditional0E,
                OriginalGuardState.Timed0F,
                OriginalGuardState.Timed10,
                OriginalGuardState.RecoverMove11,
                OriginalGuardState.WaitAnimation12,
                OriginalGuardState.Transition13,
                OriginalGuardState.Periodic14,
                OriginalGuardState.Pain15
            };

            foreach (var state in confirmed)
            {
                Assert(OriginalRuntimeState.IsConfirmedAutonomousState(state),
                    "confirmed autonomous GUARD state missing from ownership gate: 0x" +
                    ((byte)state).ToString("X2"));
            }

            Assert(confirmed.Length == OriginalRuntime.GuardStateCount,
                "autonomous GUARD coverage must contain all 22 dispatcher states.");
        }

        static void TestObjectDefinitionCatalog()
        {
            Assert(Marshal.SizeOf<OriginalObjectDefinitionRecord>() ==
                   OriginalRuntime.ObjectResourceHeaderBytes,
                "IMG resource header must be 0x5A bytes.");
            Assert(Marshal.SizeOf<OriginalImageFrameRuntimeRecord>() ==
                   OriginalRuntime.ObjectResourceEntryBytes,
                "IMG runtime frame descriptor must be 10 bytes.");

            byte[] header = new byte[OriginalRuntime.ObjectResourceHeaderBytes];
            header[0x00] = 0x34;
            header[0x01] = 0x12;
            header[0x02] = 6;
            header[0x34] = 0x12;
            header[0x35] = 0x04;
            header[0x36] = 0x20;
            header[0x37] = 0x03;
            header[0x38] = 0x30;
            header[0x39] = 0x02;

            var parsed = OriginalObjectDefinitionCatalog.ParseHeader(header, 0);
            Assert(parsed.Interval == 0x1234 &&
                   parsed.FrameCount == 6 &&
                   parsed.State02Sequence == 0x0412 &&
                   parsed.State03Sequence == 0x0320 &&
                   parsed.State04Sequence == 0x0230,
                "IMG resource-header field decode mismatch.");

            Assert(OriginalObjectDefinitionCatalog.HeaderOffset(0, true) == 0x800 &&
                   OriginalObjectDefinitionCatalog.HeaderOffset(1, true) == 0x85A &&
                   OriginalObjectDefinitionCatalog.HeaderOffset(0, false) == 0x6200,
                "IMG dual-bank 0x5A header offset mismatch.");

            byte[] tableA = new byte[6 * OriginalRuntime.ObjectResourceEntryBytes];

            var catalog = new OriginalObjectDefinitionCatalog();
            Assert(catalog.TryGetOrAdd(
                       0x12345678, header, tableA, out byte first) &&
                   first == 0 &&
                   catalog.Count == 1,
                "first IMG resource allocation mismatch.");

            byte[] changedSameKey = (byte[])header.Clone();
            changedSameKey[0x34] = 0xFF;
            Assert(catalog.TryGetOrAdd(
                       0x12345678, changedSameKey, tableA, out byte duplicate) &&
                   duplicate == 0 &&
                   catalog.Count == 1,
                "equal source key must deduplicate to original definition id.");

            byte[] headerB = (byte[])header.Clone();
            headerB[0x34] = 0x44;
            Assert(catalog.TryGetOrAdd(
                       0x87654321, headerB, tableA, out byte second) &&
                   second == 1 &&
                   catalog.Count == 2,
                "new source key must allocate the next definition id.");

            Assert(catalog.TryGetGuardStateWord(
                       first, OriginalGuardState.Active02, out ushort seq02) &&
                   seq02 == 0x0412,
                "state 02 IMG header word lookup mismatch.");
            Assert(catalog.TryGetGuardStateWord(
                       first, OriginalGuardState.Detection03, out ushort seq03) &&
                   seq03 == 0x0320,
                "state 03 IMG header word lookup mismatch.");
            Assert(catalog.TryGetGuardStateWord(
                       first, OriginalGuardState.DetectionAttack04, out ushort seq04) &&
                   seq04 == 0x0230,
                "state 04 IMG header word lookup mismatch.");

            Assert(!catalog.TryGetOrAdd(
                       0x11111111,
                       header,
                       new byte[OriginalRuntime.ObjectResourceEntryBytes],
                       out _),
                "frame table byte count must match header frame-count*10.");
        }

        static void TestImgResourceLoader()
        {
            const byte objectId = 128;
            int streamOffset = OriginalRuntime.ImgFirstFrameStreamOffset;
            byte[] img = new byte[streamOffset + 10 + (32 * 32)];

            int directoryOffset = 0x0400 + objectId * 4;
            img[directoryOffset + 0] = (byte)streamOffset;
            img[directoryOffset + 1] = (byte)(streamOffset >> 8);
            img[directoryOffset + 2] = (byte)(streamOffset >> 16);
            img[directoryOffset + 3] = (byte)(streamOffset >> 24);

            int headerOffset =
                OriginalObjectDefinitionCatalog.HeaderOffset(objectId, false);
            img[headerOffset + 0x00] = 0x78;
            img[headerOffset + 0x01] = 0x56;
            img[headerOffset + 0x02] = 1;
            img[headerOffset + 0x34] = 0x12;
            img[headerOffset + 0x35] = 0x04;
            img[headerOffset + 0x36] = 0x20;
            img[headerOffset + 0x37] = 0x03;
            img[headerOffset + 0x38] = 0x30;
            img[headerOffset + 0x39] = 0x02;

            img[streamOffset + 0] = 32;
            img[streamOffset + 1] = 32;
            img[streamOffset + 10] = 0x2A;
            img[streamOffset + 10 + (32 * 32) - 1] = 0x5C;

            Assert(OriginalImgDefinitionLoader.TryReadObjectDefinition(
                       img,
                       objectId,
                       out uint sourceKey,
                       out byte[] headerBytes,
                       out byte[] runtimeFrames),
                "synthetic IMG object resource must parse.");

            Assert(sourceKey == (uint)streamOffset &&
                   headerBytes.Length == OriginalRuntime.ObjectResourceHeaderBytes &&
                   runtimeFrames.Length == OriginalRuntime.ObjectResourceEntryBytes,
                "synthetic IMG object resource sizes mismatch.");

            var definition =
                OriginalObjectDefinitionCatalog.Parse(headerBytes, 0);
            Assert(definition.Interval == 0x5678 &&
                   definition.FrameCount == 1 &&
                   definition.State02Sequence == 0x0412 &&
                   definition.State03Sequence == 0x0320 &&
                   definition.State04Sequence == 0x0230,
                "synthetic IMG definition values mismatch.");

            var frame =
                OriginalImgResourceReader.DecodeRuntimeFrame(runtimeFrames, 0);
            Assert(frame.Width == 32 &&
                   frame.Height == 32 &&
                   frame.PixelDataFileOffset == (uint)(streamOffset + 10) &&
                   frame.CachedPixelsPointer == 0,
                "synthetic IMG runtime frame conversion mismatch.");

            var catalog = new OriginalObjectDefinitionCatalog();
            Assert(OriginalImgDefinitionLoader.TryRegisterObjectDefinition(
                       img,
                       objectId,
                       catalog,
                       out byte definitionId) &&
                   definitionId == 0 &&
                   catalog.TryGetHeader(definitionId, out var registered) &&
                   registered.Interval == 0x5678 &&
                   registered.State03Sequence == 0x0320,
                "synthetic IMG definition registration mismatch.");

            Assert(catalog.TryGetBitmapFrame(
                       definitionId,
                       0,
                       img,
                       out BitmapImage bitmap) &&
                   bitmap.width == 32 &&
                   bitmap.height == 32 &&
                   bitmap.data[0, 0] == 0x2A &&
                   bitmap.data[31, 31] == 0x5C,
                "original IMG bitmap-frame decode mismatch.");

            Assert(catalog.TryGetBitmapFrame(
                       definitionId,
                       0,
                       img,
                       out BitmapImage cachedBitmap) &&
                   Object.ReferenceEquals(bitmap, cachedBitmap),
                "original IMG bitmap frame must be cached per definition/frame.");
        }

        static void TestOriginalMapTables()
        {
            byte[] map = new byte[
                OriginalMapTables.HeaderBytes +
                OriginalMapTables.LevelBytes];

            map[0] = 1; // one level
            map[1] = 0;

            // Raw wall ids -> translated runtime classes.
            map[0x0002 + 5] = 0x01; // hard wall
            map[0x0002 + 6] = 0x31; // dynamic door family
            map[0x0002 + 7] = 0x00; // empty

            // Raw object ids -> translated runtime classes.
            map[0x0102 + 9] = 0x08; // blocking object
            map[0x0102 + 10] = 0x2A; // blocking family + LOS exception
            map[0x0102 + 11] = 0x00;

            // Eight raw IDs sharing one class exercise FUN_2398-style variant math.
            for (int i = 20; i <= 27; i++)
                map[0x0102 + i] = 0x0F;

            int level = OriginalMapTables.HeaderBytes;

            // (0,0) hard wall + empty object
            map[level + 0] = 5;
            map[level + 1] = 11;

            // (1,0) dynamic door + empty object
            map[level + 2] = 6;
            map[level + 3] = 11;

            // (2,0) empty wall + blocking object
            map[level + 4] = 7;
            map[level + 5] = 9;

            // (3,0) empty wall + class-2A LOS exception
            map[level + 6] = 7;
            map[level + 7] = 10;

            var tables = OriginalMapTables.Parse(map, 0);

            Assert(tables.LevelCount == 1 &&
                   tables.WallId[0, 0] == 5 &&
                   tables.ObjectId[2, 0] == 9,
                "MAP header/raw cell parse mismatch.");

            Assert(tables.TryGetObjectClassAndVariant(
                       24, out byte mappedClass, out byte mappedVariant) &&
                   mappedClass == 0x0F &&
                   mappedVariant == 4,
                "MAP object class/variant lookup mismatch.");

            Assert(tables.WallClassAt(0, 0) == 0x01 &&
                   (tables.WallPropertyAt(0, 0) &
                    OriginalMapTables.WallHardBlock) != 0,
                "MAP wall class/property translation mismatch.");

            Assert((tables.WallPropertyAt(1, 0) &
                    OriginalMapTables.WallDynamicDoor) != 0,
                "MAP dynamic-door property mismatch.");

            Assert((tables.ObjectPropertyAt(2, 0) &
                    OriginalMapTables.ObjectBlocksMovementOrLos) != 0,
                "MAP blocking-object property mismatch.");

            Assert((tables.ObjectPropertyAt(3, 0) &
                    OriginalMapTables.ObjectLosPassThroughException) != 0,
                "MAP class-2A LOS exception property mismatch.");

            Assert(tables.IsPerceptionIntermediateBlocked(
                       0, 0, false, (x, y) => true),
                "hard wall must block D50A perception.");

            Assert(tables.IsPerceptionIntermediateBlocked(
                       1, 0, false, (x, y) => false),
                "closed dynamic door must block D50A perception.");

            Assert(!tables.IsPerceptionIntermediateBlocked(
                       1, 0, false, (x, y) => true),
                "open dynamic door must allow D50A perception.");

            Assert(tables.IsPerceptionIntermediateBlocked(
                       2, 0, true, (x, y) => true),
                "secondary object blocker must block D50A perception.");

            Assert(!tables.IsPerceptionIntermediateBlocked(
                       3, 0, true, (x, y) => true),
                "class-2A object must use D50A LOS pass-through exception.");

            Assert(tables.IsPlayerObjectBlocked84F4(2, 0),
                "ordinary object property 0x02 must block player collision.");
            Assert(tables.IsPlayerObjectBlocked84F4(3, 0),
                "84F4 player collision must ignore the D50A class-2A LOS exception.");

            Assert(!tables.IsPerceptionIntermediateBlocked(
                       2, 0, false, (x, y) => true),
                "secondary-cell disabled mode must ignore object blocker.");

            Assert(tables.TryFindNearestReachableDoor(
                       3, 0, (x, y) => false,
                       out int nearestDoorX, out int nearestDoorY) &&
                   nearestDoorX == 1 &&
                   nearestDoorY == 0,
                "strategy-1 nearest reachable door selection mismatch.");
        }

        static void TestOriginalWallRuntime()
        {
            byte[] mapBytes = new byte[
                OriginalMapTables.HeaderBytes +
                OriginalMapTables.LevelBytes];
            mapBytes[0] = 1;

            // raw wall 1 -> runtime door class 0x31
            mapBytes[0x0002 + 1] = 0x31;
            int level = OriginalMapTables.HeaderBytes;
            int cell = (1 + 1 * 64) * 2;
            mapBytes[level + cell] = 1;

            var map = OriginalMapTables.Parse(mapBytes, 0);

            var first = new OriginalRendererCore.Vec
            {
                Flags = OriginalMapTables.WallDynamicDoor,
                RenderClass = 0x31,
                Orientation = 0,
                X1 = 64,
                Y1 = 64,
                X2 = 128,
                Y2 = 64
            };
            var second = new OriginalRendererCore.Vec
            {
                Flags = OriginalMapTables.WallDynamicDoor,
                RenderClass = 0x31,
                Orientation = 0,
                X1 = 64,
                Y1 = 64,
                X2 = 128,
                Y2 = 128
            };

            var vectors = new System.Collections.Generic.List<OriginalRendererCore.Vec>
            {
                first,
                second
            };
            var walls = new OriginalWallRuntime(map, vectors);
            var door = walls.FindPairedWall(1, 1);

            Assert(door != null &&
                   door.State == 1 &&
                   door.Timer == 0 &&
                   door.TargetX == second.X1 &&
                   door.TargetY == second.Y1 &&
                   door.Runtime.State == 1 &&
                   door.Runtime.Timer == 0 &&
                   door.Runtime.TargetX == second.X1 &&
                   door.Runtime.TargetY == second.Y1 &&
                   door.Runtime.Latch == 0,
                "FUN_14A8 paired-wall controller initialization mismatch.");

            Assert(map.IsPlayerPrimaryWallBlocked84F4(1, 1, walls),
                "closed state-1 dynamic door must block 84F4 player collision.");

            Assert(!walls.DoorAllowsSight(1, 1),
                "closed state-1 door must block D50A sight.");

            Assert(walls.TogglePairedWall(1, 1, 2, false) &&
                   door.State == 2,
                "FUN_188A state-1 door must enter opening state 2.");

            walls.TickPairedWallMotion();
            Assert(door.State == 2,
                "paired-wall motion must remain opening before target is reached.");

            Assert(walls.ForceState(1, 1, 0) &&
                   door.State == 0 &&
                   door.Runtime.State == 0 &&
                   walls.DoorAllowsSight(1, 1) &&
                   !map.IsPlayerPrimaryWallBlocked84F4(1, 1, walls),
                "paired-wall state 0 force/passability mismatch.");

            Assert(walls.SetLatchedPassable(1, 1) &&
                   door.State == 4 &&
                   door.Runtime.State == 4 &&
                   walls.DoorAllowsSight(1, 1),
                "paired-wall state 4 latched/passable mismatch.");

            Assert(walls.TryGetDoorRuntimeRecord(
                       1, 1, out var doorRecord) &&
                   doorRecord.State == 4 &&
                   doorRecord.TargetX == door.TargetX &&
                   doorRecord.TargetY == door.TargetY,
                "door runtime record export mismatch.");
        }

        static void TestExplodingWallRuntime()
        {
            string path = Path.GetTempFileName();

            try
            {
                byte[] img = new byte[0xBD00];

                void WriteUInt16(int offset, ushort value)
                {
                    img[offset + 0] = (byte)value;
                    img[offset + 1] = (byte)(value >> 8);
                }

                void WriteUInt32(int offset, uint value)
                {
                    img[offset + 0] = (byte)value;
                    img[offset + 1] = (byte)(value >> 8);
                    img[offset + 2] = (byte)(value >> 16);
                    img[offset + 3] = (byte)(value >> 24);
                }

                void DefineWall(
                    byte wallId,
                    uint stream,
                    ushort interval,
                    byte frameCount)
                {
                    WriteUInt32(wallId * 4, stream);

                    int seq = 0x0800 + wallId * 0x5A;
                    WriteUInt16(seq + 0, interval);
                    img[seq + 2] = frameCount;
                    img[seq + 3] = 0;

                    int p = checked((int)stream);
                    for (int i = 0; i < frameCount; i++)
                    {
                        img[p + 0] = 1;
                        img[p + 1] = 1;
                        img[p + 10] = (byte)(0x20 + i);
                        p += 11;
                    }
                }

                DefineWall(1, 0xBC00, 10, 1); // class 0x2E source
                DefineWall(2, 0xBC20, 10, 2); // class 0x2D explosion
                DefineWall(3, 0xBC50, 15, 2); // class 0x2F source

                File.WriteAllBytes(path, img);

                var ex1 = new OriginalRendererCore.Vec
                {
                    WallId = 1,
                    RenderClass = 0x2E
                };
                var ex2 = new OriginalRendererCore.Vec
                {
                    WallId = 3,
                    RenderClass = 0x2F
                };

                var vectors =
                    new List<OriginalRendererCore.Vec>
                    {
                        ex1,
                        ex2
                    };

                var runtime =
                    new OriginalImgWallRuntime(
                        path,
                        vectors);

                Assert(runtime.EnsureWallIdCached(
                           2,
                           out byte explosionCache),
                    "class-0x2D wall cache must support explicit preload.");

                byte ex2OriginalCache = ex2.TextureSet;

                runtime.BeginExplodingWall(
                    ex1,
                    0x2E,
                    explosionCache,
                    100);

                Assert(ex1.RenderClass == 0x2D &&
                       ex1.AnimationFrame == 0 &&
                       ex1.TextureSet == explosionCache &&
                       ex1.RuntimeTimer == 110,
                    "WALL_EX1 must enter class 0x2D at frame 0 with generic explosion cache.");

                runtime.BeginExplodingWall(
                    ex2,
                    0x2F,
                    explosionCache,
                    100);

                Assert(ex2.RenderClass == 0x2D &&
                       ex2.AnimationFrame == 1 &&
                       ex2.TextureSet == ex2OriginalCache &&
                       ex2.RuntimeTimer == 115,
                    "WALL_EX2 must enter class 0x2D at frame 1 and retain its sequence cache.");

                bool completed = false;

                runtime.UpdateAfterVisibleSpan(
                    ex1,
                    109,
                    null,
                    _ => completed = true);

                Assert(!completed &&
                       ex1.AnimationFrame == 0,
                    "exploding wall must not advance before deadline.");

                runtime.UpdateAfterVisibleSpan(
                    ex1,
                    110,
                    null,
                    _ => completed = true);

                Assert(!completed &&
                       ex1.AnimationFrame == 1 &&
                       ex1.RuntimeTimer == 120,
                    "exploding wall first due frame/deadline mismatch.");

                runtime.UpdateAfterVisibleSpan(
                    ex1,
                    120,
                    null,
                    _ => completed = true);

                Assert(completed &&
                       ex1.AnimationFrame == 1 &&
                       ex1.RuntimeTimer == 130,
                    "class-0x2D completion must clamp to final frame and reschedule.");
            }
            finally
            {
                File.Delete(path);
            }
        }

        static void TestOriginalRandom()
        {
            OriginalRandom.ResetToOriginalSeed();

            ushort[] expected =
            {
                41, 18467, 6334, 26500, 19169,
                15724, 11478, 29358, 26962, 24464
            };

            for (int i = 0; i < expected.Length; i++)
            {
                Assert(OriginalRandom.Next() == expected[i],
                    "Win16 CRT rand sequence mismatch at " + i);
            }

            OriginalRandom.SetSeed(1);
            Assert(OriginalRandom.Seed == 1 &&
                   OriginalRandom.Next() == 41,
                "srand(1) compatibility mismatch.");

            OriginalRandom.ResetToOriginalSeed();
        }

        static void TestDamageMatrix()
        {
            Assert(OriginalDamage.ApplyClassWeaponTransform(
                       80, 0x0C, (byte)OriginalWeaponSelector.SingleShotLaser) == 10,
                "class 0x0C divide-8 transform mismatch.");

            Assert(OriginalDamage.ApplyClassWeaponTransform(
                       80, 0x0D, (byte)OriginalWeaponSelector.MagicWand) == 40,
                "class 0x0D Wand divide-2 transform mismatch.");
            Assert(OriginalDamage.ApplyClassWeaponTransform(
                       80, 0x0D, (byte)OriginalWeaponSelector.SilverPistol) == 10,
                "class 0x0D Silver divide-8 transform mismatch.");

            Assert(OriginalDamage.ApplyClassWeaponTransform(
                       80, 0x0E, (byte)OriginalWeaponSelector.MagicWand) == 40,
                "class 0x0E Wand divide-2 transform mismatch.");

            Assert(OriginalDamage.ApplyClassWeaponTransform(
                       512, 0x0F, (byte)OriginalWeaponSelector.SingleShotLaser) == 2,
                "Baddie non-Wand divide-256 transform mismatch.");
            Assert(OriginalDamage.ApplyClassWeaponTransform(
                       80, 0x0F, (byte)OriginalWeaponSelector.MagicWand) == 40,
                "Baddie Wand divide-2 transform mismatch.");

            Assert(OriginalDamage.ApplyClassWeaponTransform(
                       80, 0x12, (byte)OriginalWeaponSelector.MagicWand) == 20,
                "gargoyle divide-4 transform mismatch.");

            Assert(OriginalDamage.ApplyClassWeaponTransform(
                       80, 0x1A, (byte)OriginalWeaponSelector.MagicWand) == 40,
                "Ghost Wand transform mismatch.");
            Assert(OriginalDamage.ApplyClassWeaponTransform(
                       80, 0x1A, (byte)OriginalWeaponSelector.SilverPistol) == 0,
                "Ghost non-Wand immunity mismatch.");

            Assert(OriginalDamage.ApplyClassWeaponTransform(
                       80, 0x16, (byte)OriginalWeaponSelector.MagicWand, 2) == 0,
                "Hamerstein must be immune when gate != 3.");
            Assert(OriginalDamage.ApplyClassWeaponTransform(
                       80, 0x16, (byte)OriginalWeaponSelector.MagicWand, 3) == 3,
                "Hamerstein gate value 3 must yield literal damage 3.");
        }

        static void TestScriptProgress51A6()
        {
            OriginalRuntimeState.GuardAttackClass16FullDamageOverride = false;
            Assert(OriginalRuntimeState.StoryProgress51A6 == 0,
                "51A6 test must start at phase 0.");

            Assert(!OriginalRuntimeState.ApplyScriptTouchProgress51A6(
                       1, 6, 0x47) &&
                   OriginalRuntimeState.StoryProgress51A6 == 0,
                "episode 1 level 6 must not activate 51A6.");

            Assert(!OriginalRuntimeState.ApplyScriptTouchProgress51A6(
                       1, 7, 0x48),
                "class 0x48 must not activate the 51A6 damage gate.");

            Assert(OriginalRuntimeState.ApplyScriptTouchProgress51A6(
                       1, 7, 0x47) &&
                   OriginalRuntimeState.StoryProgress51A6 == 1,
                "episode 1 level 7 class 0x47 must activate 51A6.");

            Assert(!OriginalRuntimeState.ApplyScriptTouchProgress51A6(
                       3, 10, 0x47),
                "already-active 51A6 must not retrigger phase 1.");

            Assert(OriginalRuntimeState.AdvanceScriptProgress51A6ToPhase2() &&
                   OriginalRuntimeState.StoryProgress51A6 == 2,
                "51A6 phase 1 -> 2 transition mismatch.");

            Assert(!OriginalRuntimeState.AdvanceScriptProgress51A6ToPhase2(),
                "51A6 phase 2 must not advance again.");

            // Restore neutral startup state for later tests.
            OriginalRuntimeState.GuardAttackClass16FullDamageOverride = false;
        }

        static void TestWeaponJamScriptBFD8()
        {
            OriginalRuntimeState.WeaponRuntime.ResetNewGame();

            Assert(!OriginalRuntimeState.ApplyWeaponJamScriptTouchBFD8(
                       1, 8, 0x47) &&
                   !OriginalRuntimeState.WeaponRuntime.Jammed,
                "E1M8 class 0x47 must not set DAT_4C2E.");

            Assert(OriginalRuntimeState.ApplyWeaponJamScriptTouchBFD8(
                       1, 9, 0x47) &&
                   OriginalRuntimeState.WeaponRuntime.Jammed,
                "E1M9 class 0x47 must set DAT_4C2E.");

            Assert(!OriginalRuntimeState.ApplyWeaponJamScriptTouchBFD8(
                       1, 9, 0x47) &&
                   OriginalRuntimeState.WeaponRuntime.Jammed,
                "repeated E1M9 class 0x47 must not retrigger an already-set latch.");

            Assert(!OriginalRuntimeState.ApplyWeaponJamScriptTouchBFD8(
                       2, 9, 0x48) &&
                   OriginalRuntimeState.WeaponRuntime.Jammed,
                "jam script must remain episode-1 specific.");

            Assert(OriginalRuntimeState.ApplyWeaponJamScriptTouchBFD8(
                       1, 9, 0x48) &&
                   !OriginalRuntimeState.WeaponRuntime.Jammed,
                "E1M9 class 0x48 must clear DAT_4C2E.");

            Assert(!OriginalRuntimeState.ApplyWeaponJamScriptTouchBFD8(
                       1, 9, 0x48),
                "repeated E1M9 class 0x48 must not retrigger a cleared latch.");
        }

        static void TestGuardSounds()
        {
            Assert(OriginalGuardSounds.AlertSoundId(0x08, 0) == 0x22,
                "Bat alert sound mismatch.");
            Assert(OriginalGuardSounds.AlertSoundId(0x09, 2) == 0x3A,
                "Frankenstein randomized alert sound mismatch.");
            Assert(OriginalGuardSounds.AlertSoundId(0x0F, 1) == 0x37,
                "Baddie randomized alert sound mismatch.");
            Assert(OriginalGuardSounds.AlertSoundId(0x1F, 0) == 0x3D,
                "Alien #2 alert sound mismatch.");
            Assert(OriginalGuardSounds.AlertSoundId(0x15, 0) == 0,
                "Penelope alert selector should take the default zero path.");

            Assert(OriginalGuardSounds.AttackSoundId(0x09, 0) == 0x41,
                "Frankenstein attack sound mismatch.");
            Assert(OriginalGuardSounds.AttackSoundId(0x0F, 2) == 0x19,
                "Baddie randomized attack sound mismatch.");
            Assert(OriginalGuardSounds.AttackSoundId(0x13, 0) == 0x40,
                "Garden gargoyle attack sound mismatch.");
            Assert(OriginalGuardSounds.AttackSoundId(0x1B, 3) == 0x4E,
                "Goldie randomized attack sound mismatch.");
            Assert(OriginalGuardSounds.AttackSoundId(0x08, 0) == 0,
                "Bat attack selector should take the default zero path.");

            Assert(OriginalGuardSounds.DeathSoundId(0x08, 0) == 0x23,
                "Bat death sound mismatch.");
            Assert(OriginalGuardSounds.DeathSoundId(0x0F, 2) == 0x0D,
                "Baddie randomized death sound mismatch.");
            Assert(OriginalGuardSounds.DeathSoundId(0x1A, 0) == 0x4A,
                "Ghost death sound mismatch.");
            Assert(OriginalGuardSounds.DeathSoundId(0x1D, 0) == 0x09,
                "Demon death sound mismatch.");
            Assert(OriginalGuardSounds.DeathSoundId(0x16, 0) == 0,
                "Hamerstein death selector should take the default zero path.");
        }

        static void TestGuardToPlayerDamage()
        {
            Assert(OriginalDamage.OriginalRoundedSqrt(0) == 0 &&
                   OriginalDamage.OriginalRoundedSqrt(1) == 1 &&
                   OriginalDamage.OriginalRoundedSqrt(2) == 2 &&
                   OriginalDamage.OriginalRoundedSqrt(4) == 2 &&
                   OriginalDamage.OriginalRoundedSqrt(5) == 3 &&
                   OriginalDamage.OriginalRoundedSqrt(9) == 3 &&
                   OriginalDamage.OriginalRoundedSqrt(11) == 4,
                "original integer square-root rounding mismatch.");

            Assert(OriginalDamage.ComputeGuardAttackDistanceMetric(
                       64, 64, 128, 128) == 2,
                "one-tile diagonal GUARD distance must round to 2.");
            Assert(OriginalDamage.ComputeGuardAttackDistanceMetric(
                       64, 64, 192, 128) == 3,
                "2x1 tile GUARD distance must round to 3.");

            var diagonal = OriginalDamage.ComputeGuardToPlayerFromWorld(
                64, 64, 128, 128, 0x0C, 1, false, 0);
            Assert(diagonal.DistanceSeed == 50 &&
                   diagonal.DifficultyTransformed == 50,
                "world-coordinate GUARD damage distance mismatch.");

            var normal = OriginalDamage.ComputeGuardToPlayer(
                5, 0x0C, 1, false, 0);
            Assert(normal.DistanceSeed == 20 &&
                   normal.ClassTransformed == 20 &&
                   normal.DifficultyTransformed == 20 &&
                   normal.StoredByte == 20,
                "GUARD-to-player normal damage seed mismatch.");

            var hardRandom = OriginalDamage.ComputeGuardToPlayer(
                10, 0x08, 2, false, 7);
            Assert(hardRandom.ClassTransformed == 7 &&
                   hardRandom.DifficultyTransformed == 14,
                "class 0x08 RNG/hard scaling mismatch.");

            var easyQuarter = OriginalDamage.ComputeGuardToPlayer(
                1, 0x0B, 0, false, 0);
            Assert(easyQuarter.DistanceSeed == 100 &&
                   easyQuarter.ClassTransformed == 25 &&
                   easyQuarter.DifficultyTransformed == 12,
                "class 0x0B quarter/easy scaling mismatch.");

            Assert(OriginalDamage.ComputeGuardToPlayer(
                       1, 0x16, 1, false, 0).DifficultyTransformed == 0x21,
                "class 0x16 normal gate damage must be 33.");
            Assert(OriginalDamage.ComputeGuardToPlayer(
                       1, 0x16, 1, true, 0).DifficultyTransformed == 100,
                "class 0x16 full-damage gate must be 100.");

            Assert(OriginalDamage.ComputeGuardToPlayer(
                       1, 0x19, 2, false, 0).DifficultyTransformed == 200,
                "class 0x19 hard damage must be 200.");

            Assert(OriginalDamage.ComputeGuardToPlayer(
                       4, 0x1F, 1, false, 0).DifficultyTransformed == 12,
                "class 0x1F must follow the original default half-seed branch.");
        }

        static void TestPackedGuardSequences()
        {
            var guard = new OriginalGuardRecord
            {
                State = (byte)OriginalGuardState.Active02
            };
            var obj = new OriginalObjectRecord();

            Assert(OriginalGuardDispatcher.BeginState02AlertSequence(
                       ref guard, ref obj, 0x0412) ==
                   OriginalGuardDispatchResult.Transitioned,
                "state 02 alert sequence should transition.");
            Assert(guard.DefinitionValue == 0x0412 &&
                   obj.Component03 == 0x12 &&
                   guard.Timer == 3 &&
                   guard.State == (byte)OriginalGuardState.AnimationTimer &&
                   guard.NextState == (byte)OriginalGuardState.Detection03,
                "state 02 packed sequence fields mismatch.");

            Assert(OriginalGuardDispatcher.CompleteDeferredState(ref guard) ==
                   OriginalGuardDispatchResult.Completed &&
                   guard.State == (byte)OriginalGuardState.Detection03,
                "state 02 sequence must return through state 03.");

            Assert(OriginalGuardDispatcher.BeginState03AttackSequence(
                       ref guard, ref obj, 0x0320) ==
                   OriginalGuardDispatchResult.Transitioned,
                "state 03 attack sequence should transition.");
            Assert(obj.Component03 == 0x20 &&
                   guard.Timer == 2 &&
                   guard.NextState == (byte)OriginalGuardState.DetectionAttack04,
                "state 03 packed sequence fields mismatch.");

            OriginalGuardDispatcher.CompleteDeferredState(ref guard);
            Assert(guard.State == (byte)OriginalGuardState.DetectionAttack04,
                "state 03 sequence must return through state 04.");

            Assert(OriginalGuardDispatcher.BeginState04RecoverySequence(
                       ref guard, ref obj, 0x0230) ==
                   OriginalGuardDispatchResult.Transitioned,
                "state 04 recovery sequence should transition.");
            Assert(obj.Component03 == 0x30 &&
                   guard.Timer == 1 &&
                   guard.NextState == (byte)OriginalGuardState.Transition05,
                "state 04 packed sequence fields mismatch.");

            OriginalGuardDispatcher.CompleteDeferredState(ref guard);
            Assert(guard.State == (byte)OriginalGuardState.Transition05,
                "state 04 sequence must return through state 05.");
        }

        static void TestExactAnimationSchedulers()
        {
            var guard = new OriginalGuardRecord
            {
                DefinitionValue = 0x0304,
                Timer = 4,
                State = (byte)OriginalGuardState.AnimationTimer,
                NextState = (byte)OriginalGuardState.Detection03
            };
            var obj = new OriginalObjectRecord
            {
                Component03 = 4
            };

            Assert(OriginalGuardDispatcher.TickAnimationTimer(
                       ref guard, ref obj) ==
                   OriginalGuardDispatchResult.Waiting &&
                   unchecked((byte)obj.Component03) == 5 &&
                   guard.Timer == 3,
                "state 00 first frame/timer tick mismatch.");

            OriginalGuardDispatcher.TickAnimationTimer(ref guard, ref obj);
            Assert(unchecked((byte)obj.Component03) == 6 &&
                   guard.Timer == 2,
                "state 00 second frame/timer tick mismatch.");

            OriginalGuardDispatcher.TickAnimationTimer(ref guard, ref obj);
            Assert(unchecked((byte)obj.Component03) == 4 &&
                   guard.Timer == 1,
                "state 00 sequence must loop to first frame.");

            Assert(OriginalGuardDispatcher.TickAnimationTimer(
                       ref guard, ref obj) ==
                   OriginalGuardDispatchResult.Transitioned &&
                   guard.State == (byte)OriginalGuardState.Detection03 &&
                   guard.Timer == 0,
                "state 00 must return through nextState when timer expires.");

            guard = new OriginalGuardRecord
            {
                DefinitionValue = 0x0304,
                Timer = 2,
                State = (byte)OriginalGuardState.WaitAnimation12,
                NextState = (byte)OriginalGuardState.Active07
            };
            obj = new OriginalObjectRecord
            {
                Component03 = 4,
                Runtime1A = 11
            };

            Assert(OriginalGuardDispatcher.TickWaitAnimation12(
                       ref guard, ref obj) ==
                   OriginalGuardDispatchResult.Waiting &&
                   unchecked((byte)obj.Component03) == 5 &&
                   obj.Runtime1A == 6 &&
                   guard.Timer == 1,
                "state 12 first tick mismatch.");

            Assert(OriginalGuardDispatcher.TickWaitAnimation12(
                       ref guard, ref obj) ==
                   OriginalGuardDispatchResult.Waiting &&
                   unchecked((byte)obj.Component03) == 6 &&
                   obj.Runtime1A == 1 &&
                   guard.Timer == 0,
                "state 12 must wait for OBJECT+1A after timer reaches zero.");

            Assert(OriginalGuardDispatcher.TickWaitAnimation12(
                       ref guard, ref obj) ==
                   OriginalGuardDispatchResult.Transitioned &&
                   obj.Runtime1A == 0 &&
                   guard.State == (byte)OriginalGuardState.Active07,
                "state 12 completion gate mismatch.");

            guard = new OriginalGuardRecord
            {
                DefinitionValue = 0x0304,
                State = (byte)OriginalGuardState.Pain15,
                NextState = (byte)OriginalGuardState.Active07
            };
            obj = new OriginalObjectRecord
            {
                Component03 = 4
            };

            Assert(OriginalGuardDispatcher.TickPainReaction(
                       ref guard, ref obj) ==
                   OriginalGuardDispatchResult.Waiting &&
                   unchecked((byte)obj.Component03) == 5,
                "state 15 first frame mismatch.");
            Assert(OriginalGuardDispatcher.TickPainReaction(
                       ref guard, ref obj) ==
                   OriginalGuardDispatchResult.Waiting &&
                   unchecked((byte)obj.Component03) == 6,
                "state 15 final-frame advance mismatch.");
            Assert(OriginalGuardDispatcher.TickPainReaction(
                       ref guard, ref obj) ==
                   OriginalGuardDispatchResult.Transitioned &&
                   guard.State == (byte)OriginalGuardState.Active07,
                "state 15 must return through nextState at final frame.");
        }

        static void TestHitTransitionRouting()
        {
            var definition = new OriginalObjectDefinitionRecord
            {
                ReactionSequence0 = 0x0204,
                DeathSequence0 = 0x0408,
                DeathSequence7 = 0x020C
            };

            int rngCalls = 0;
            Func<ushort> zeroRng = () =>
            {
                rngCalls++;
                return 0;
            };

            var guard = new OriginalGuardRecord
            {
                State = (byte)OriginalGuardState.Active02,
                Strategy = 0,
                ResultOctant = 3
            };
            var obj = new OriginalObjectRecord();

            Assert(OriginalGuardDispatcher.BeginNonLethalHitTransition(
                       ref guard,
                       ref obj,
                       definition,
                       zeroRng,
                       false) == OriginalGuardHitTransition.Pain15 &&
                   guard.State == (byte)OriginalGuardState.Pain15 &&
                   guard.NextState == (byte)OriginalGuardState.Active02 &&
                   guard.DefinitionValue == 0x0204 &&
                   unchecked((byte)obj.Component03) == 0x04 &&
                   guard.ResultOctant == 8,
                "ordinary hit must enter state 15 with selected reaction sequence.");

            guard = new OriginalGuardRecord
            {
                State = (byte)OriginalGuardState.Active07,
                Strategy = 0
            };
            obj = new OriginalObjectRecord();
            Assert(OriginalGuardDispatcher.BeginNonLethalHitTransition(
                       ref guard,
                       ref obj,
                       definition,
                       zeroRng,
                       true) == OriginalGuardHitTransition.AnimationTo05 &&
                   guard.State == (byte)OriginalGuardState.AnimationTimer &&
                   guard.NextState == (byte)OriginalGuardState.Transition05 &&
                   guard.Unknown17 == 1,
                "states 7/8/15 must animate through state 0 to state 5.");

            guard = new OriginalGuardRecord
            {
                State = (byte)OriginalGuardState.MoveThen03,
                Strategy = 2
            };
            obj = new OriginalObjectRecord();
            Assert(OriginalGuardDispatcher.BeginNonLethalHitTransition(
                       ref guard,
                       ref obj,
                       definition,
                       zeroRng,
                       false) == OriginalGuardHitTransition.AnimationTo08 &&
                   guard.State == (byte)OriginalGuardState.AnimationTimer &&
                   guard.NextState == (byte)OriginalGuardState.Move08,
                "strategy 2 hit must animate through state 0 to state 8.");

            guard = new OriginalGuardRecord
            {
                State = (byte)OriginalGuardState.Active02,
                Strategy = 4
            };
            obj = new OriginalObjectRecord();
            rngCalls = 0;
            Assert(OriginalGuardDispatcher.BeginNonLethalHitTransition(
                       ref guard,
                       ref obj,
                       definition,
                       zeroRng,
                       false) == OriginalGuardHitTransition.ReactionSkipped &&
                   rngCalls == 0 &&
                   guard.State == (byte)OriginalGuardState.Active02 &&
                   guard.ResultOctant == 8,
                "strategy 4 must skip the selector and reaction animation.");

            guard = new OriginalGuardRecord
            {
                State = (byte)OriginalGuardState.Detection03,
                Strategy = 3
            };
            obj = new OriginalObjectRecord();
            rngCalls = 0;
            Assert(OriginalGuardDispatcher.BeginNonLethalHitTransition(
                       ref guard,
                       ref obj,
                       definition,
                       zeroRng,
                       false) == OriginalGuardHitTransition.ReactionSkipped &&
                   rngCalls == 1 &&
                   guard.State == (byte)OriginalGuardState.Detection03,
                "state 3 must consume reaction selector but skip local pain animation.");

            guard = new OriginalGuardRecord
            {
                State = (byte)OriginalGuardState.Active07,
                Strength = 50,
                ResultOctant = 1
            };
            obj = new OriginalObjectRecord
            {
                Runtime1A = 0
            };
            Assert(OriginalGuardDispatcher.BeginLethalHitTransition(
                       ref guard,
                       ref obj,
                       definition,
                       zeroRng) == OriginalGuardHitTransition.DeathAnimation &&
                   guard.Strength == 0 &&
                   guard.State == (byte)OriginalGuardState.AnimationTimer &&
                   guard.NextState == (byte)OriginalGuardState.DeathFinalize09 &&
                   guard.DefinitionValue == 0x0408 &&
                   guard.Timer == 3 &&
                   unchecked((byte)obj.Component03) == 0x08,
                "lethal hit without OBJECT+1A must use state 0 -> state 9.");

            guard = new OriginalGuardRecord
            {
                State = (byte)OriginalGuardState.Active07,
                Strength = 50,
                ResultOctant = 1
            };
            obj = new OriginalObjectRecord
            {
                Runtime1A = 10
            };
            Assert(OriginalGuardDispatcher.BeginLethalHitTransition(
                       ref guard,
                       ref obj,
                       definition,
                       zeroRng) == OriginalGuardHitTransition.DeathAnimation &&
                   guard.State == (byte)OriginalGuardState.WaitAnimation12 &&
                   guard.NextState == (byte)OriginalGuardState.DeathFinalize09,
                "lethal hit with OBJECT+1A must use state 12 -> state 9.");

            rngCalls = 0;
            Assert(OriginalGuardDispatcher.TrySelectDeathSequence(
                       definition,
                       0,
                       zeroRng,
                       out int deathSelector,
                       out ushort deathSequence) &&
                   deathSelector == 7 &&
                   deathSequence == 0x020C &&
                   rngCalls == 0,
                "resoct zero must prefer valid death slot 7 without RNG.");

            var lowByteOnly = new OriginalObjectDefinitionRecord
            {
                ReactionSequence0 = 0x00FF
            };
            Assert(!lowByteOnly.TryGetReactionSequence(0, out _),
                "sequence validity must require a nonzero high/frame-count byte.");

            guard = new OriginalGuardRecord
            {
                State = (byte)OriginalGuardState.DeathFinalize09,
                Strength = 0
            };
            obj = new OriginalObjectRecord
            {
                ObjectClass = OriginalRuntime.DraculaPhase1Class,
                DefinitionId = 9,
                Runtime1A = 0
            };

            OriginalGuardDispatcher.ApplyDraculaPhase2Reset(
                ref guard,
                ref obj,
                3);

            Assert(obj.DefinitionId == 3 &&
                   obj.ObjectClass == OriginalRuntime.DraculaBatPhase2Class &&
                   obj.Runtime1A == 0x23 &&
                   guard.Strength == OriginalRuntime.GuardInitialStrength &&
                   guard.State == (byte)OriginalGuardState.Move08 &&
                   guard.NextState == (byte)OriginalGuardState.Active02 &&
                   guard.Timer == 1,
                "Dracula phase reset must occur after state-09 finalization.");
        }

        static void TestAttackGate()
        {
            var guard = new OriginalGuardRecord
            {
                TransitionControl = 0
            };
            var obj = new OriginalObjectRecord
            {
                WorldX = 100,
                WorldY = 200
            };

            Assert(OriginalGuardDispatcher.TryEvaluateAttackGate(
                       ref guard, ref obj, 164, 264, false, out bool adjacent) &&
                   adjacent &&
                   guard.Unknown17 == 0 &&
                   guard.Unknown18 == 1,
                "attack mode 0 must use one-tile proximity.");

            Assert(OriginalGuardDispatcher.TryEvaluateAttackGate(
                       ref guard, ref obj, 165, 264, true, out bool outside) &&
                   !outside &&
                   guard.Unknown17 == 1 &&
                   guard.Unknown18 == 0,
                "attack mode 0 must ignore perception outside one tile.");

            guard.TransitionControl = 1;
            Assert(OriginalGuardDispatcher.TryEvaluateAttackGate(
                       ref guard, ref obj, 300, 400, true, out bool visible) &&
                   visible,
                "attack mode 1 must use perception result.");

            guard.TransitionControl = 2;
            Assert(OriginalGuardDispatcher.TryEvaluateAttackGate(
                       ref guard, ref obj, 300, 400, false, out bool hidden) &&
                   !hidden,
                "attack mode 2 must use perception result.");

            guard.TransitionControl = 3;
            Assert(!OriginalGuardDispatcher.TryEvaluateAttackGate(
                       ref guard, ref obj, 100, 200, true, out _),
                "unrecovered attack modes above 2 must stay rejected.");
        }

        static void TestPerceptionPrefilter()
        {
            var guard = new OriginalGuardRecord
            {
                Octant = 2 // east
            };
            var obj = new OriginalObjectRecord
            {
                WorldX = 64,
                WorldY = 64
            };

            Assert(OriginalGuardDispatcher.GuardPerceptionPrefilter(
                       ref guard, ref obj, 128, 64, false),
                "east-facing guard must accept east target.");
            Assert(!OriginalGuardDispatcher.GuardPerceptionPrefilter(
                       ref guard, ref obj, 0, 64, false),
                "east-facing guard must reject west target.");
            Assert(OriginalGuardDispatcher.GuardPerceptionPrefilter(
                       ref guard, ref obj, 128, 0, false),
                "east-facing guard must accept northeast target.");
            Assert(!OriginalGuardDispatcher.GuardPerceptionPrefilter(
                       ref guard, ref obj, 64, -512, false),
                "perception must reject targets beyond 8 tiles on an axis.");
            Assert(OriginalGuardDispatcher.GuardPerceptionPrefilter(
                       ref guard, ref obj, 0, 64, true),
                "ignoreFacing mode must bypass the octant mask.");

            bool traceCalled = false;
            bool perceived = OriginalGuardDispatcher.EvaluateGuardPerception(
                ref guard,
                ref obj,
                128,
                64,
                true,
                false,
                (startX, startY, dx, dy, maxSteps, secondary) =>
                {
                    traceCalled = true;
                    Assert(startX == 1 &&
                           startY == 1 &&
                           dx == 1 &&
                           dy == 0 &&
                           maxSteps == 8 &&
                           secondary,
                        "perception line-trace arguments mismatch.");
                    return true;
                });

            Assert(perceived && traceCalled,
                "perception wrapper must return the line-trace result after prefilter.");

            traceCalled = false;
            perceived = OriginalGuardDispatcher.EvaluateGuardPerception(
                ref guard,
                ref obj,
                0,
                64,
                false,
                false,
                (a, b, dx, dy, n, secondary) =>
                {
                    traceCalled = true;
                    return true;
                });

            Assert(!perceived && !traceCalled,
                "failed FOV prefilter must not call the map trace.");
        }

        static void TestGuardGridTrace()
        {
            int blockerCalls = 0;
            Assert(OriginalGuardDispatcher.TraceGuardGridLine(
                       0, 0, 1, 0, 8, false,
                       (x, y, secondary) =>
                       {
                           blockerCalls++;
                           return true;
                       }) &&
                   blockerCalls == 0,
                "destination cell must be accepted before intermediate blocking.");

            blockerCalls = 0;
            Assert(!OriginalGuardDispatcher.TraceGuardGridLine(
                       0, 0, 3, 0, 8, true,
                       (x, y, secondary) =>
                       {
                           blockerCalls++;
                           return x == 1 && y == 0 && secondary;
                       }) &&
                   blockerCalls == 1,
                "first blocked intermediate cell must abort D50A trace.");

            blockerCalls = 0;
            Assert(OriginalGuardDispatcher.TraceGuardGridLine(
                       0, 0, 2, 2, 8, false,
                       (x, y, secondary) =>
                       {
                           blockerCalls++;
                           return false;
                       }) &&
                   blockerCalls == 1,
                "2x2 diagonal trace must inspect only the intermediate cell.");

            Assert(!OriginalGuardDispatcher.TraceGuardGridLine(
                       0, 0, 3, 0, 2, false,
                       (x, y, secondary) => false),
                "trace must fail when target is beyond maxSteps.");

            Assert(OriginalGuardDispatcher.TraceGuardGridLine(
                       5, 5, -3, -1, 8, false,
                       (x, y, secondary) => false),
                "negative-delta Bresenham trace mismatch.");
        }

        static void TestGuardLosCellFlags()
        {
            Assert(!OriginalGuardDispatcher.GuardLosCellBlocks(
                       0x00, 0x00, false, false),
                "ordinary LOS cell must remain transparent.");

            Assert(OriginalGuardDispatcher.GuardLosCellBlocks(
                       0x02 | 0x04, 0x00, false, true),
                "primary 0x04 must block special LOS cells.");

            Assert(OriginalGuardDispatcher.GuardLosCellBlocks(
                       0x02 | 0x08, 0x00, false, false),
                "primary 0x08 must block when linked runtime record is not passable.");
            Assert(!OriginalGuardDispatcher.GuardLosCellBlocks(
                       0x02 | 0x08, 0x00, false, true),
                "primary 0x08 must pass when linked runtime record is passable.");

            Assert(OriginalGuardDispatcher.GuardLosCellBlocks(
                       0x00, 0x02, true, true),
                "secondary 0x02 must block when secondary checks are enabled.");
            Assert(!OriginalGuardDispatcher.GuardLosCellBlocks(
                       0x00, 0x02 | 0x20, true, true),
                "secondary 0x20 must exempt secondary 0x02 from blocking.");
            Assert(!OriginalGuardDispatcher.GuardLosCellBlocks(
                       0x00, 0x02, false, true),
                "secondary flags must be ignored when secondary checks are disabled.");

            Assert(OriginalGuardDispatcher.GuardLosRuntimeRecordPassable(0) &&
                   OriginalGuardDispatcher.GuardLosRuntimeRecordPassable(4) &&
                   !OriginalGuardDispatcher.GuardLosRuntimeRecordPassable(1) &&
                   !OriginalGuardDispatcher.GuardLosRuntimeRecordPassable(3),
                "FUN_1476 runtime state predicate mismatch.");
        }

        static void TestCellFlagGenerators()
        {
            Assert(OriginalGuardDispatcher.BuildPrimaryCellFlags(0x00) == 0x00,
                "primary class 00 flags mismatch.");
            Assert(OriginalGuardDispatcher.BuildPrimaryCellFlags(0x01) == 0x07,
                "primary class 01 must set 0x01/0x02/0x04.");
            Assert(OriginalGuardDispatcher.BuildPrimaryCellFlags(0x2E) == 0x17,
                "primary class 2E must add 0x10.");
            Assert(OriginalGuardDispatcher.BuildPrimaryCellFlags(0x31) == 0x0B,
                "primary class 31 must set 0x01/0x02/0x08.");
            Assert(OriginalGuardDispatcher.BuildPrimaryCellFlags(0x47) == 0x40,
                "primary class 47 must set only 0x40.");

            Assert(OriginalGuardDispatcher.BuildSecondaryCellFlags(0x04) == 0x40,
                "secondary class 04 flags mismatch.");
            Assert(OriginalGuardDispatcher.BuildSecondaryCellFlags(0x08) == 0x0B,
                "secondary class 08 must set 0x01/0x02/0x08.");
            Assert(OriginalGuardDispatcher.BuildSecondaryCellFlags(0x2A) == 0x23,
                "secondary class 2A must set 0x01/0x02/0x20.");
            Assert(OriginalGuardDispatcher.BuildSecondaryCellFlags(0x2F) == 0x05,
                "secondary class 2F must set 0x01/0x04.");
            Assert(OriginalGuardDispatcher.BuildSecondaryCellFlags(0x3D) == 0x05,
                "secondary class 3D must set 0x01/0x04.");
        }

        static void TestOriginalSoundEventMapping()
        {
            Assert(SoundEffect.OriginalEventToPhysicalSlot(0) == 32,
                "original sound event 0 must map to physical SND slot 32.");
            Assert(SoundEffect.OriginalEventToPhysicalSlot(2) == 34,
                "original sound event 2 must map to first loaded SFX slot 34.");
            Assert(SoundEffect.OriginalEventToPhysicalSlot(35) == 67,
                "Bat death event 35 must map to physical SND slot 67.");
            Assert(SoundEffect.OriginalEventToPhysicalSlot(78) == 110,
                "highest recovered Win16 event 78 must map to slot 110.");
        }

        static void TestProjectileRuntime()
        {
            Assert(OriginalRuntime.MaxProjectiles == 8,
                "original projectile pool capacity must be 8.");
            Assert(OriginalRuntime.ProjectileRuntimeStride == 42,
                "original projectile stride must be 42 bytes.");

            Assert(OriginalProjectileRuntime.WeaponUsesProjectile(0),
                "single-shot laser must use projectile pool.");
            Assert(OriginalProjectileRuntime.WeaponUsesProjectile(1),
                "Magic Wand must use projectile pool.");
            Assert(!OriginalProjectileRuntime.WeaponUsesProjectile(2),
                "Silver Pistol is hitscan and must not use projectile pool.");
            Assert(OriginalProjectileRuntime.WeaponUsesProjectile(3),
                "continuous laser must use projectile pool.");

            Assert(OriginalProjectileRuntime.TryGetSequenceOffsets(
                       1, out var wandOffsets) &&
                   wandOffsets.Flight == 2 && wandOffsets.Impact == 3,
                "Magic Wand projectile sequence offsets mismatch.");

            Assert(OriginalProjectileRuntime.HitsGuard(9, -9, 0, 0),
                "projectile guard hit tolerance must include +/-9.");
            Assert(!OriginalProjectileRuntime.HitsGuard(10, 0, 0, 0),
                "projectile guard hit tolerance must exclude 10.");

            Assert(OriginalProjectileRuntime.NeedsProjection(21, 0, 0, 0),
                "projectile projection threshold must trigger beyond 20.");
            Assert(!OriginalProjectileRuntime.NeedsProjection(20, -20, 0, 0),
                "projectile projection threshold must not trigger at +/-20.");

            Assert(OriginalProjectileRuntime.AngleFromDirection(0, -1) == 0 &&
                   OriginalProjectileRuntime.AngleFromDirection(1, 0) == 90 &&
                   OriginalProjectileRuntime.AngleFromDirection(0, 1) == 180 &&
                   OriginalProjectileRuntime.AngleFromDirection(-1, 0) == 270,
                "projectile direction-to-original-angle mapping mismatch.");

            var trajectory = new OriginalProjectileRecord
            {
                State = (byte)OriginalProjectileState.Flying,
                RenderObject = new OriginalObjectRecord
                {
                    WorldX = 100,
                    WorldY = 100,
                    Runtime1A = 5
                }
            };
            OriginalProjectileRuntime.ConfigureDdaFromAngle(
                ref trajectory,
                90);

            int visited = 0;
            Assert(!OriginalProjectileRuntime.AdvanceTrajectoryAndCollide(
                       ref trajectory,
                       20,
                       (x, y) =>
                       {
                           visited++;
                           return false;
                       }),
                "unblocked projectile trajectory must not report collision.");
            Assert(visited == 20 &&
                   trajectory.RenderObject.WorldX == 120 &&
                   trajectory.RenderObject.WorldY == 100 &&
                   trajectory.RenderObject.Runtime1A == 6,
                "projectile 20-substep trajectory/update-anchor mismatch.");

            trajectory = new OriginalProjectileRecord
            {
                State = (byte)OriginalProjectileState.Flying,
                RenderObject = new OriginalObjectRecord
                {
                    WorldX = 100,
                    WorldY = 100,
                    Runtime1A = 19
                }
            };
            OriginalProjectileRuntime.ConfigureDdaFromAngle(
                ref trajectory,
                90);

            visited = 0;
            Assert(OriginalProjectileRuntime.AdvanceTrajectoryAndCollide(
                       ref trajectory,
                       20,
                       (x, y) =>
                       {
                           visited++;
                           return x == 105;
                       }),
                "projectile trajectory must stop on the first colliding substep.");
            Assert(visited == 5 &&
                   trajectory.RenderObject.WorldX == 105 &&
                   trajectory.RenderObject.WorldY == 100 &&
                   trajectory.RenderObject.Runtime1A == 20,
                "projectile collision stop / vertical-anchor clamp mismatch.");

            var dda = new OriginalProjectileRecord
            {
                State = (byte)OriginalProjectileState.Flying,
                RenderObject = new OriginalObjectRecord
                {
                    WorldX = 100,
                    WorldY = 100
                }
            };

            OriginalProjectileRuntime.ConfigureDdaFromAngle(ref dda, 0);
            Assert(dda.XIsMajorAxis == 0 &&
                   dda.StepY == -1 &&
                   dda.LineError == -1024 &&
                   dda.MinorErrorStep == 0 &&
                   dda.MajorErrorFixup == -2048,
                "north Q10 DDA initialization mismatch.");
            Assert(OriginalProjectileRuntime.AdvanceDdaSubstep(ref dda) &&
                   dda.RenderObject.WorldX == 100 &&
                   dda.RenderObject.WorldY == 99,
                "north DDA substep mismatch.");

            dda = new OriginalProjectileRecord
            {
                State = (byte)OriginalProjectileState.Flying,
                RenderObject = new OriginalObjectRecord
                {
                    WorldX = 100,
                    WorldY = 100
                }
            };
            OriginalProjectileRuntime.ConfigureDdaFromAngle(ref dda, 90);
            Assert(dda.XIsMajorAxis == 1 &&
                   dda.StepX == 1,
                "east Q10 DDA initialization mismatch.");
            Assert(OriginalProjectileRuntime.AdvanceDdaSubstep(ref dda) &&
                   dda.RenderObject.WorldX == 101 &&
                   dda.RenderObject.WorldY == 100,
                "east DDA substep mismatch.");

            dda = new OriginalProjectileRecord
            {
                State = (byte)OriginalProjectileState.Flying,
                RenderObject = new OriginalObjectRecord
                {
                    WorldX = 100,
                    WorldY = 100
                }
            };
            OriginalProjectileRuntime.ConfigureDdaFromAngle(ref dda, 45);
            Assert(OriginalProjectileRuntime.AdvanceDdaSubstep(ref dda) &&
                   dda.RenderObject.WorldX == 101 &&
                   dda.RenderObject.WorldY == 99,
                "45-degree DDA substep must advance both axes.");

            var pool = new OriginalProjectileRecord[OriginalRuntime.MaxProjectiles];
            Assert(OriginalProjectileRuntime.FirstFreeSlot(pool) == 0,
                "empty projectile pool must allocate slot 0.");
            pool[0].State = (byte)OriginalProjectileState.Flying;
            Assert(OriginalProjectileRuntime.FirstFreeSlot(pool) == 1,
                "projectile pool must advance to next free slot.");

            Assert(OriginalProjectileRuntime.TryAllocateAndInitialize(
                       pool, 0, 64, 128, 20, out int allocated) &&
                   allocated == 1 &&
                   pool[1].State == (byte)OriginalProjectileState.Flying,
                "projectile allocator must initialize the first free slot.");

            for (int i = 0; i < pool.Length; i++)
                pool[i].State = (byte)OriginalProjectileState.Flying;

            Assert(!OriginalProjectileRuntime.TryAllocateAndInitialize(
                       pool, 0, 64, 128, 20, out int fullSlot) &&
                   fullSlot == -1,
                "full projectile pool must reject allocation before ammo use.");

            var projectile = new OriginalProjectileRecord();
            Assert(OriginalProjectileRuntime.InitializeSpawn(
                       ref projectile, 1, 100, 200, 10),
                "Magic Wand projectile spawn must initialize.");
            Assert(projectile.State == (byte)OriginalProjectileState.Flying &&
                   projectile.RenderObject.Component03 == 0 &&
                   projectile.RenderObject.DefinitionId == 12 &&
                   projectile.RenderObject.Flags == 0x01 &&
                   projectile.RenderObject.ObjectClass == 5 &&
                   projectile.RenderObject.WorldX == 100 &&
                   projectile.RenderObject.WorldY == 200 &&
                   projectile.RenderObject.Runtime1A == 5,
                "projectile spawn template mismatch.");

            OriginalProjectileRuntime.AdvanceAnimationFrame(ref projectile, 2);
            Assert(projectile.RenderObject.Component03 == 1,
                "flight animation first frame advance mismatch.");
            OriginalProjectileRuntime.AdvanceAnimationFrame(ref projectile, 2);
            Assert(projectile.RenderObject.Component03 == 0,
                "flight animation must loop.");

            Assert(OriginalProjectileRuntime.EnterImpact(
                       ref projectile, 1, 10),
                "Magic Wand projectile impact transition must initialize.");
            Assert(projectile.State == (byte)OriginalProjectileState.Impact &&
                   projectile.RenderObject.Component03 == 0 &&
                   projectile.RenderObject.DefinitionId == 13 &&
                   (projectile.RenderObject.Flags & 0x10) != 0,
                "projectile impact transition mismatch.");

            OriginalProjectileRuntime.AdvanceAnimationFrame(ref projectile, 2);
            Assert(projectile.State == (byte)OriginalProjectileState.Impact &&
                   projectile.RenderObject.Component03 == 1,
                "impact animation intermediate frame mismatch.");
            OriginalProjectileRuntime.AdvanceAnimationFrame(ref projectile, 2);
            Assert(projectile.State == (byte)OriginalProjectileState.Free,
                "impact animation must free slot after final frame.");

            projectile = new OriginalProjectileRecord();
            OriginalProjectileRuntime.InitializeSpawnWithDefinition(
                ref projectile,
                7,
                100,
                200,
                1000,
                40);

            Assert(projectile.State == (byte)OriginalProjectileState.Flying &&
                   projectile.RenderObject.DefinitionId == 7 &&
                   projectile.RenderObject.RuntimeValue == 1040 &&
                   projectile.RenderObject.Runtime1A == 5,
                "exact projectile spawn definition/deadline mismatch.");

            Assert(!OriginalProjectileRuntime.AdvanceAnimationIfDue(
                       ref projectile,
                       1039,
                       2,
                       40) &&
                   projectile.RenderObject.Component03 == 0 &&
                   projectile.RenderObject.RuntimeValue == 1040,
                "projectile animation must not advance before its absolute deadline.");

            Assert(OriginalProjectileRuntime.AdvanceAnimationIfDue(
                       ref projectile,
                       1040,
                       2,
                       40) &&
                   projectile.RenderObject.Component03 == 1 &&
                   projectile.RenderObject.RuntimeValue == 1080,
                "due projectile animation must advance exactly one frame and reschedule.");

            OriginalProjectileRuntime.EnterImpactWithDefinition(
                ref projectile,
                8,
                1100,
                25);

            Assert(projectile.State == (byte)OriginalProjectileState.Impact &&
                   projectile.RenderObject.DefinitionId == 8 &&
                   projectile.RenderObject.Component03 == 0 &&
                   projectile.RenderObject.RuntimeValue == 1125 &&
                   (projectile.RenderObject.Flags & 0x10) != 0,
                "exact projectile impact transition/deadline mismatch.");

            Assert(!OriginalProjectileRuntime.AdvanceAnimationIfDue(
                       ref projectile,
                       1124,
                       2,
                       25) &&
                   projectile.State == (byte)OriginalProjectileState.Impact,
                "impact must remain allocated before its deadline.");

            Assert(OriginalProjectileRuntime.AdvanceAnimationIfDue(
                       ref projectile,
                       1125,
                       2,
                       25) &&
                   projectile.State == (byte)OriginalProjectileState.Impact &&
                   projectile.RenderObject.Component03 == 1 &&
                   projectile.RenderObject.RuntimeValue == 1150,
                "impact first due frame/deadline mismatch.");

            Assert(OriginalProjectileRuntime.AdvanceAnimationIfDue(
                       ref projectile,
                       1150,
                       2,
                       25) &&
                   projectile.State == (byte)OriginalProjectileState.Free,
                "impact must free the slot only after its final frame.");
        }

        static void TestDelayState()
        {
            var guard = new OriginalGuardRecord
            {
                State = (byte)OriginalGuardState.Delay,
                Timer = 2
            };

            Assert(OriginalGuardDispatcher.TickDelay(ref guard) ==
                   OriginalGuardDispatchResult.Waiting,
                "state 01 should wait while timer remains.");
            Assert(guard.Timer == 1 &&
                   guard.State == (byte)OriginalGuardState.Delay,
                "state 01 first countdown mismatch.");

            Assert(OriginalGuardDispatcher.TickDelay(ref guard) ==
                   OriginalGuardDispatchResult.Transitioned,
                "state 01 should transition when timer expires.");
            Assert(guard.Timer == 0 &&
                   guard.State == (byte)OriginalGuardState.Active02,
                "state 01 must transition to state 02.");
        }

        static void TestState13Movement()
        {
            sbyte[] expectedX = { 0, 8, 8, 0, 0, -8, -8, 0 };
            sbyte[] expectedY = { -8, 0, 0, 8, 8, 0, 0, -8 };

            for (byte facing = 0; facing < 8; facing++)
            {
                OriginalGuardDispatcher.GetState13Movement(
                    facing, out sbyte x, out sbyte y);
                Assert(x == expectedX[facing] && y == expectedY[facing],
                    "state 13 facing lookup mismatch at " + facing);
            }

            var guard = new OriginalGuardRecord
            {
                State = (byte)OriginalGuardState.Active07,
                Strategy = 3,
                Octant = 1
            };

            OriginalGuardDispatcher.EnterStrategy3TimedMove(
                ref guard, 1); // 1 % 0x50 + 8 = 9

            Assert(guard.Timer == 9 &&
                   guard.State == (byte)OriginalGuardState.Transition13 &&
                   guard.MoveX == 8 &&
                   guard.MoveY == 0,
                "state 13 entry mismatch.");

            var obj = new OriginalObjectRecord
            {
                WorldX = 100,
                WorldY = 100
            };

            Assert(OriginalGuardDispatcher.TickState13(
                       ref guard, ref obj, (x, y) => true) ==
                   OriginalGuardDispatchResult.SoundPoint,
                "state 13 should emit sound point at remaining timer 8.");

            int moved = 0;
            while (guard.State == (byte)OriginalGuardState.Transition13)
            {
                var result = OriginalGuardDispatcher.TickState13(
                    ref guard, ref obj, (x, y) => true);
                if (result == OriginalGuardDispatchResult.Moved)
                    moved++;
            }

            Assert(moved == 8, "state 13 must perform eight movement attempts.");
            Assert(obj.WorldX == 164 && obj.WorldY == 100,
                "state 13 total displacement must be 64 world units.");
            Assert(guard.Strategy == 0 &&
                   guard.State == (byte)OriginalGuardState.Active02,
                "state 13 must clear strategy and return to state 02.");

            guard = new OriginalGuardRecord
            {
                State = (byte)OriginalGuardState.Transition13,
                Strategy = 3,
                Timer = 8,
                MoveX = 8,
                MoveY = 0
            };
            obj = new OriginalObjectRecord
            {
                WorldX = 100,
                WorldY = 100
            };

            int blockedAttempts = 0;
            while (guard.State == (byte)OriginalGuardState.Transition13)
            {
                var result = OriginalGuardDispatcher.TickState13(
                    ref guard, ref obj, (x, y) => false);

                if (result == OriginalGuardDispatchResult.MovementBlocked)
                    blockedAttempts++;
            }

            Assert(blockedAttempts == 8,
                "blocked state 13 must still consume all eight movement attempts.");
            Assert(obj.WorldX == 100 && obj.WorldY == 100,
                "blocked state 13 must not commit coordinates.");
            Assert(guard.Strategy == 0 &&
                   guard.State == (byte)OriginalGuardState.Active02,
                "blocked state 13 must still terminate normally.");
        }

        static void TestState11Movement()
        {
            var definition = new OriginalObjectDefinitionRecord
            {
                DirectionalB4 = 0x0204
            };

            var guard = new OriginalGuardRecord
            {
                State = (byte)OriginalGuardState.RecoverMove11,
                Strategy = 1,
                Timer = 2,
                MoveX = 8,
                MoveY = 0,
                Octant = 2,
                Control = 1,
                ResultOctant = 0xFF
            };

            var obj = new OriginalObjectRecord
            {
                WorldX = 100,
                WorldY = 100,
                Component03 = 4
            };

            Assert(OriginalGuardDispatcher.TickState11Movement(
                       ref guard,
                       ref obj,
                       definition,
                       200,
                       100,
                       (x, y) => false,
                       () => 0,
                       out var firstMove) ==
                   OriginalGuardDispatchResult.Moved,
                "state 11 first movement tick must remain active.");

            Assert(firstMove.PositionCommitted &&
                   obj.WorldX == 108 &&
                   obj.WorldY == 100 &&
                   guard.Timer == 1 &&
                   guard.State == (byte)OriginalGuardState.RecoverMove11 &&
                   guard.Strategy == 1,
                "state 11 first tick movement/countdown mismatch.");

            Assert(OriginalGuardDispatcher.TickState11Movement(
                       ref guard,
                       ref obj,
                       definition,
                       200,
                       100,
                       (x, y) => false,
                       () => 0,
                       out _) ==
                   OriginalGuardDispatchResult.Transitioned,
                "state 11 timer expiry must transition.");

            Assert(obj.WorldX == 116 &&
                   guard.Timer == 0 &&
                   guard.MoveX == 0 &&
                   guard.MoveY == 0 &&
                   guard.Strategy == 0 &&
                   guard.State == (byte)OriginalGuardState.Active07,
                "state 11 completion must clear temporary door movement and return to state 07.");
        }

        static void TestMovementPlanning()
        {
            int[] dx = { 0, 8, 8, 8, 0, -8, -8, -8 };
            int[] dy = { -8, -8, 0, 8, 8, 8, 0, -8 };
            for (byte expected = 0; expected < 8; expected++)
            {
                var facing = new OriginalGuardRecord
                {
                    Octant = 6
                };
                OriginalGuardDispatcher.UpdateOctantFromMovement(
                    ref facing,
                    dx[expected],
                    dy[expected]);
                Assert(facing.Octant == expected,
                    "FUN_6E66 octant mapping mismatch at " + expected);
            }

            var unchanged = new OriginalGuardRecord
            {
                Octant = 5
            };
            OriginalGuardDispatcher.UpdateOctantFromMovement(
                ref unchanged, 0, 0);
            Assert(unchanged.Octant == 5,
                "zero movement must preserve the previous octant.");

            ushort[] rngValues = { 2, 7 };
            int rngIndex = 0;
            Func<ushort> rng = () => rngValues[rngIndex++];

            var guard = new OriginalGuardRecord
            {
                Strategy = 0,
                Unknown17 = 1,
                Unknown18 = 0
            };
            var obj = new OriginalObjectRecord
            {
                WorldX = 64,
                WorldY = 64
            };

            Assert(OriginalGuardDispatcher.PlanStrategy0Movement(
                       ref guard,
                       ref obj,
                       192,
                       128,
                       1,
                       rng) == OriginalGuardDispatchResult.Transitioned &&
                   guard.MoveX == 8 &&
                   guard.MoveY == 8 &&
                   guard.Timer == 15 &&
                   guard.State == (byte)OriginalGuardState.MoveThen03 &&
                   guard.Octant == 3 &&
                   rngIndex == 2,
                "strategy-0 perceived movement plan mismatch.");

            guard = new OriginalGuardRecord
            {
                Strategy = 0,
                Unknown17 = 0,
                Unknown18 = 0,
                MoveX = -8,
                MoveY = 8
            };
            obj = new OriginalObjectRecord
            {
                WorldX = 128,
                WorldY = 128
            };
            rngIndex = 0;
            rngValues = new ushort[] { 0 };

            Assert(OriginalGuardDispatcher.PlanStrategy0Movement(
                       ref guard,
                       ref obj,
                       128,
                       256,
                       1,
                       rng) == OriginalGuardDispatchResult.Transitioned &&
                   guard.MoveX == 8 &&
                   guard.MoveY == 8 &&
                   guard.Timer == 24 &&
                   rngIndex == 1,
                "strategy-0 no-perception timer/direction branch mismatch.");

            guard = new OriginalGuardRecord
            {
                Strategy = 0,
                Unknown17 = 1,
                Unknown18 = 1
            };
            obj = new OriginalObjectRecord
            {
                WorldX = 128,
                WorldY = 128
            };
            rngIndex = 0;
            rngValues = new ushort[] { 2 };

            OriginalGuardDispatcher.PlanStrategy0Movement(
                ref guard,
                ref obj,
                64,
                64,
                2,
                rng);

            Assert(guard.MoveX == -8 &&
                   guard.MoveY == -8 &&
                   guard.Timer == 8 &&
                   guard.Octant == 7 &&
                   rngIndex == 1,
                "one-tile proximity must force timer 8 before difficulty scaling.");

            guard = new OriginalGuardRecord
            {
                Strategy = 0,
                Unknown17 = 1,
                Unknown18 = 0
            };
            obj = new OriginalObjectRecord
            {
                WorldX = 64,
                WorldY = 64
            };
            rngIndex = 0;
            rngValues = new ushort[] { 2, 7 };
            OriginalGuardDispatcher.PlanStrategy0Movement(
                ref guard,
                ref obj,
                192,
                128,
                2,
                rng);
            Assert(guard.Timer == 7,
                "hard difficulty must halve 15-tick perceived movement timer.");

            guard = new OriginalGuardRecord
            {
                Strategy = 2,
                MoveX = -8,
                MoveY = 0
            };
            rngIndex = 0;
            rngValues = new ushort[] { 7 };
            Assert(OriginalGuardDispatcher.PlanStrategy2Movement(
                       ref guard,
                       rng) == OriginalGuardDispatchResult.Transitioned &&
                   guard.Timer == 15 &&
                   guard.State == (byte)OriginalGuardState.MoveThen03 &&
                   guard.Octant == 6,
                "strategy-2 movement plan mismatch.");

            guard = new OriginalGuardRecord
            {
                Strategy = 1,
                Strength = 126,
                MoveX = 0,
                MoveY = -8,
                Octant = 0
            };
            obj = new OriginalObjectRecord
            {
                WorldX = 96,
                WorldY = 96
            };

            Assert(OriginalGuardDispatcher.PlanStrategy1Movement(
                       ref guard,
                       ref obj,
                       0,
                       0,
                       1,
                       null,
                       true,
                       192,
                       64) == OriginalGuardDispatchResult.Transitioned &&
                   guard.MoveX == 8 &&
                   guard.MoveY == 0 &&
                   guard.Timer == 0x10 &&
                   guard.State == (byte)OriginalGuardState.MoveThen03 &&
                   guard.Octant == 2,
                "strategy-1 low-HP retreat-door movement mismatch.");

            guard = new OriginalGuardRecord
            {
                Strategy = 1,
                Strength = 126,
                MoveX = -8,
                MoveY = 8,
                Octant = 5
            };
            obj = new OriginalObjectRecord
            {
                WorldX = 96,
                WorldY = 96
            };

            OriginalGuardDispatcher.PlanStrategy1Movement(
                ref guard,
                ref obj,
                0,
                0,
                1,
                null,
                false,
                0,
                0);

            Assert(guard.MoveX == -8 &&
                   guard.MoveY == 8 &&
                   guard.Timer == 0x10 &&
                   guard.Octant == 5,
                "strategy-1 with no reachable door must keep prior vector.");

            guard = new OriginalGuardRecord
            {
                Strategy = 1,
                Strength = 127,
                Unknown17 = 1,
                Unknown18 = 1
            };
            obj = new OriginalObjectRecord
            {
                WorldX = 128,
                WorldY = 128
            };
            rngIndex = 0;
            rngValues = new ushort[] { 2 };

            Assert(OriginalGuardDispatcher.PlanStrategy1Movement(
                       ref guard,
                       ref obj,
                       64,
                       64,
                       1,
                       rng,
                       false,
                       0,
                       0) == OriginalGuardDispatchResult.Transitioned &&
                   guard.MoveX == -8 &&
                   guard.MoveY == -8 &&
                   guard.Timer == 8 &&
                   rngIndex == 1,
                "strategy-1 strength 127 must use player-pursuit branch.");

            guard = new OriginalGuardRecord
            {
                Strategy = 3,
                State = (byte)OriginalGuardState.Transition05,
                Timer = 23,
                MoveX = 8,
                MoveY = -8
            };

            Assert(OriginalGuardDispatcher.PlanStrategyCurrentVectorMovement(
                       ref guard) == OriginalGuardDispatchResult.Transitioned &&
                   guard.State == (byte)OriginalGuardState.MoveThen03 &&
                   guard.Timer == 23 &&
                   guard.MoveX == 8 &&
                   guard.MoveY == -8 &&
                   guard.Octant == 1,
                "strategy 3+ must retain timer/vector through FUN_76FC common tail.");
        }

        static void TestMovementCell700A()
        {
            var definition = new OriginalObjectDefinitionRecord();
            var guard = new OriginalGuardRecord
            {
                Strategy = 0,
                State = (byte)OriginalGuardState.MoveThen03
            };
            var obj = new OriginalObjectRecord
            {
                WorldX = 100,
                WorldY = 100
            };

            var playerBlock =
                OriginalGuardDispatcher.EvaluateMovementCell700A(
                    ref guard,
                    ref obj,
                    120,
                    120,
                    100,
                    100,
                    0,
                    0,
                    default,
                    definition);
            Assert(playerBlock.Blocked,
                "FUN_700A player +/-41 box must block movement.");

            var objectBlock =
                OriginalGuardDispatcher.EvaluateMovementCell700A(
                    ref guard,
                    ref obj,
                    200,
                    200,
                    0,
                    0,
                    0,
                    OriginalMapTables.ObjectBlocksMovementOrLos,
                    default,
                    definition);
            Assert(objectBlock.Blocked,
                "FUN_700A object property 0x02 must block movement.");

            var openDoor = new OriginalDoorCollisionInfo
            {
                Exists = true,
                State = 0,
                RenderClass = 0x31
            };
            var pass =
                OriginalGuardDispatcher.EvaluateMovementCell700A(
                    ref guard,
                    ref obj,
                    200,
                    200,
                    0,
                    0,
                    OriginalMapTables.WallDynamicDoor,
                    0,
                    openDoor,
                    definition);
            Assert(!pass.Blocked &&
                   !pass.DoorToggleRequested,
                "FUN_700A door state 0 must be passable.");

            var movingDoor = openDoor;
            movingDoor.State = 2;
            var movingBlocked =
                OriginalGuardDispatcher.EvaluateMovementCell700A(
                    ref guard,
                    ref obj,
                    200,
                    200,
                    0,
                    0,
                    OriginalMapTables.WallDynamicDoor,
                    0,
                    movingDoor,
                    definition);
            Assert(movingBlocked.Blocked &&
                   !movingBlocked.DoorToggleRequested,
                "FUN_700A door states 2/3 must block without activation.");

            var restrictedDoor = openDoor;
            restrictedDoor.State = 1;
            restrictedDoor.RenderClass = 0x35;
            var restricted =
                OriginalGuardDispatcher.EvaluateMovementCell700A(
                    ref guard,
                    ref obj,
                    200,
                    200,
                    0,
                    0,
                    OriginalMapTables.WallDynamicDoor,
                    0,
                    restrictedDoor,
                    definition);
            Assert(restricted.Blocked &&
                   !restricted.DoorToggleRequested,
                "state-1 wall classes 0x33..0x3C must not be GUARD-activated.");

            var activatable = openDoor;
            activatable.State = 1;
            activatable.RenderClass = 0x31;
            var activated =
                OriginalGuardDispatcher.EvaluateMovementCell700A(
                    ref guard,
                    ref obj,
                    200,
                    200,
                    0,
                    0,
                    OriginalMapTables.WallDynamicDoor,
                    0,
                    activatable,
                    definition);
            Assert(activated.Blocked &&
                   activated.DoorToggleRequested &&
                   activated.DoorLatchRequested &&
                   !activated.EnteredRecoverMove11,
                "ordinary state-1 door must request FUN_188A and block this frame.");

            guard = new OriginalGuardRecord
            {
                Strategy = 1,
                State = (byte)OriginalGuardState.MoveThen03,
                MoveX = 8,
                MoveY = 8
            };
            obj = new OriginalObjectRecord
            {
                WorldX = 100,
                WorldY = 100
            };
            activatable.Orientation = 2;
            activatable.TargetY = 160;

            var fleeDoor =
                OriginalGuardDispatcher.EvaluateMovementCell700A(
                    ref guard,
                    ref obj,
                    140,
                    100,
                    0,
                    0,
                    OriginalMapTables.WallDynamicDoor,
                    0,
                    activatable,
                    definition);

            Assert(fleeDoor.Blocked &&
                   fleeDoor.DoorToggleRequested &&
                   fleeDoor.DoorLatchRequested &&
                   fleeDoor.EnteredRecoverMove11 &&
                   guard.State == (byte)OriginalGuardState.RecoverMove11 &&
                   guard.Timer == 0x20 &&
                   guard.MoveX == 8 &&
                   guard.MoveY == 0 &&
                   obj.WorldY == 192,
                "strategy-1 FUN_700A door interaction/state-11 transition mismatch.");
        }

        static void TestMovementCollisionCore()
        {
            Assert(OriginalGuardDispatcher.MovementCandidateTouchesPlayer(
                       100, 100, 141, 141),
                "player collision must include +/-41 world units.");
            Assert(!OriginalGuardDispatcher.MovementCandidateTouchesPlayer(
                       100, 100, 142, 100),
                "player collision must exclude 42 world units.");

            var guard = new OriginalGuardRecord
            {
                State = (byte)OriginalGuardState.MoveThen03,
                MoveX = 8,
                MoveY = 8,
                DefinitionValue = 0x0304,
                Octant = 0
            };
            var obj = new OriginalObjectRecord
            {
                WorldX = 100,
                WorldY = 100,
                Component03 = 4
            };

            var moved = OriginalGuardDispatcher.TickMovementCollisionCore(
                ref guard,
                ref obj,
                (x, y) => false,
                0);

            Assert(!moved.XBlocked &&
                   !moved.YBlocked &&
                   moved.PositionCommitted &&
                   !moved.Bounced &&
                   moved.AppliedX == 8 &&
                   moved.AppliedY == 8 &&
                   obj.WorldX == 108 &&
                   obj.WorldY == 108 &&
                   unchecked((byte)obj.Component03) == 5 &&
                   guard.Octant == 3,
                "unblocked state-6 diagonal movement mismatch.");

            guard = new OriginalGuardRecord
            {
                State = (byte)OriginalGuardState.MoveThen03,
                MoveX = 8,
                MoveY = 8,
                DefinitionValue = 0x0304
            };
            obj = new OriginalObjectRecord
            {
                WorldX = 100,
                WorldY = 100,
                Component03 = 4
            };

            var slide = OriginalGuardDispatcher.TickMovementCollisionCore(
                ref guard,
                ref obj,
                (x, y) => x == 124,
                0);

            Assert(slide.XBlocked &&
                   !slide.YBlocked &&
                   slide.PositionCommitted &&
                   obj.WorldX == 100 &&
                   obj.WorldY == 108 &&
                   guard.Octant == 4,
                "state 6 must slide along the unblocked axis.");

            guard = new OriginalGuardRecord
            {
                State = (byte)OriginalGuardState.Move08,
                MoveX = 8,
                MoveY = 8,
                DefinitionValue = 0x0304
            };
            obj = new OriginalObjectRecord
            {
                WorldX = 100,
                WorldY = 100,
                Component03 = 4
            };

            var state8Blocked = OriginalGuardDispatcher.TickMovementCollisionCore(
                ref guard,
                ref obj,
                (x, y) => x == 124,
                0);

            Assert(state8Blocked.XBlocked &&
                   !state8Blocked.YBlocked &&
                   !state8Blocked.PositionCommitted &&
                   obj.WorldX == 100 &&
                   obj.WorldY == 100 &&
                   unchecked((byte)obj.Component03) == 5 &&
                   guard.Octant == 4,
                "state 8 must reject the whole coordinate commit when one axis blocks.");

            guard = new OriginalGuardRecord
            {
                State = (byte)OriginalGuardState.MoveThen03,
                MoveX = 8,
                MoveY = 8,
                DefinitionValue = 0x0304
            };
            obj = new OriginalObjectRecord
            {
                WorldX = 100,
                WorldY = 100,
                Component03 = 6
            };

            var bounced = OriginalGuardDispatcher.TickMovementCollisionCore(
                ref guard,
                ref obj,
                (x, y) => true,
                1);

            Assert(bounced.XBlocked &&
                   bounced.YBlocked &&
                   bounced.PositionCommitted &&
                   bounced.Bounced &&
                   obj.WorldX == 100 &&
                   obj.WorldY == 100 &&
                   guard.MoveX == -8 &&
                   guard.MoveY == 8 &&
                   unchecked((byte)obj.Component03) == 4 &&
                   guard.Octant == 2,
                "double-block state-6 X bounce/compiler-byte behavior mismatch.");
        }

        static void TestDirectionalSequenceRefresh()
        {
            byte[] block = new byte[OriginalRuntime.ObjectDefinitionBytes];
            for (int i = 0; i < 8; i++)
            {
                block[0x04 + i * 2] = (byte)(0x10 + i);
                block[0x05 + i * 2] = 0x01;

                block[0x14 + i * 2] = (byte)(0x20 + i);
                block[0x15 + i * 2] = 0x02;

                block[0x24 + i * 2] = (byte)(0x30 + i);
                block[0x25 + i * 2] = 0x03;
            }

            var definition =
                OriginalObjectDefinitionCatalog.Parse(block, 0);

            for (int i = 0; i < 8; i++)
            {
                Assert(definition.GetDirectionalSequenceA(i) ==
                       (ushort)(0x0110 + i),
                    "directional bank A mismatch at " + i);
                Assert(definition.GetDirectionalSequenceB(i) ==
                       (ushort)(0x0220 + i),
                    "directional bank B mismatch at " + i);
                Assert(definition.GetDirectionalSequenceC(i) ==
                       (ushort)(0x0330 + i),
                    "directional bank C mismatch at " + i);
            }

            Assert(OriginalGuardDispatcher.ComputePlayerOctant(
                       1, 0, 0, 100, 0) == 2,
                "control-1 east player octant mismatch.");
            Assert(OriginalGuardDispatcher.ComputePlayerOctant(
                       1, 0, 0, 0, -100) == 0,
                "control-1 north player octant mismatch.");
            Assert(OriginalGuardDispatcher.ComputePlayerOctant(
                       1, 0, 0, 100, -100) == 1,
                "control-1 northeast player octant mismatch.");

            // The control-0 quantizer uses a different 45-degree split. This
            // point resolves NE there but E in the control-1 2:1-threshold path.
            Assert(OriginalGuardDispatcher.ComputePlayerOctant(
                       0, 0, 0, 100, -20) == 1,
                "control-0 biased octant mismatch.");
            Assert(OriginalGuardDispatcher.ComputePlayerOctant(
                       1, 0, 0, 100, -20) == 2,
                "control-1 2:1 octant threshold mismatch.");

            var guard = new OriginalGuardRecord
            {
                Control = 1,
                Octant = 3,
                State = (byte)OriginalGuardState.MoveThen03,
                Strategy = 0,
                ResultOctant = 0
            };
            var obj = new OriginalObjectRecord
            {
                WorldX = 0,
                WorldY = 0
            };

            Assert(OriginalGuardDispatcher.ComputeResultOctant(
                       ref guard,
                       ref obj,
                       100,
                       0) == 5,
                "relative result-octant arithmetic mismatch.");

            Assert(OriginalGuardDispatcher.GetDirectionalSequence(
                       definition,
                       (byte)OriginalGuardState.MoveThen03,
                       0,
                       5) == 0x0335,
                "state 6 normal movement must use directional bank C.");
            Assert(OriginalGuardDispatcher.GetDirectionalSequence(
                       definition,
                       (byte)OriginalGuardState.MoveThen03,
                       2,
                       5) == 0x0115,
                "state 6 strategy 2 must use directional bank A.");
            Assert(OriginalGuardDispatcher.GetDirectionalSequence(
                       definition,
                       (byte)OriginalGuardState.Move08,
                       0,
                       5) == 0x0225,
                "state 8 must use directional bank B.");
            Assert(OriginalGuardDispatcher.GetDirectionalSequence(
                       definition,
                       (byte)OriginalGuardState.Timed10,
                       0,
                       5) == 0x0225,
                "state 0x10 must use directional bank B.");
            Assert(OriginalGuardDispatcher.GetDirectionalSequence(
                       definition,
                       (byte)OriginalGuardState.Detection03,
                       0,
                       5) == 0x0115,
                "ordinary states must use directional bank A.");

            Assert(OriginalGuardDispatcher.RefreshDirectionalSequence(
                       ref guard,
                       ref obj,
                       definition,
                       100,
                       0,
                       false) == OriginalGuardDispatchResult.Transitioned &&
                   guard.ResultOctant == 5 &&
                   guard.DefinitionValue == 0x0335 &&
                   unchecked((byte)obj.Component03) == 0x35,
                "FUN_6EE0 directional refresh mismatch.");

            Assert(OriginalGuardDispatcher.RefreshDirectionalSequence(
                       ref guard,
                       ref obj,
                       definition,
                       100,
                       0,
                       false) == OriginalGuardDispatchResult.Waiting,
                "unchanged resoct must skip a non-forced directional refresh.");

            Assert(OriginalGuardDispatcher.RefreshDirectionalSequence(
                       ref guard,
                       ref obj,
                       definition,
                       100,
                       0,
                       true) == OriginalGuardDispatchResult.Transitioned,
                "forced FUN_6EE0 refresh must rewrite the sequence.");
        }

        static void TestGuardMapClassMapping()
        {
            Assert(OriginalGuardProfiles.TryClassFromMapObjectId(
                       1, 0x80, out byte e1Bat) && e1Bat == 0x08,
                "episode 1 Bat must map to class 0x08.");
            Assert(OriginalGuardProfiles.TryClassFromMapObjectId(
                       1, 0xB0, out byte e1Dracula) && e1Dracula == 0x11,
                "episode 1 Dracula must map to class 0x11.");
            Assert(OriginalGuardProfiles.TryClassFromMapObjectId(
                       1, 0xBC, out byte e1Penelope) && e1Penelope == 0x15,
                "episode 1 Penelope must map to class 0x15.");
            Assert(OriginalGuardProfiles.TryClassFromMapObjectId(
                       1, 0xC4, out byte e1Hamerstein) && e1Hamerstein == 0x16,
                "episode 1 Hamerstein must map to class 0x16.");
            Assert(OriginalGuardProfiles.TryClassFromMapObjectId(
                       1, 0xCC, out byte e1Cannon) && e1Cannon == 0x19,
                "episode 1 Cannon must map to class 0x19.");
            Assert(!OriginalGuardProfiles.TryClassFromMapObjectId(
                       1, 0x8C, out _),
                "episode 1 Dancers/GUARD26 must remain outside ordinary mapping.");

            Assert(OriginalGuardProfiles.TryClassFromMapObjectId(
                       2, 0xBC, out byte e2Robot1) && e2Robot1 == 0x17,
                "episode 2 Tall slim robot must map to class 0x17.");
            Assert(OriginalGuardProfiles.TryClassFromMapObjectId(
                       2, 0xC0, out byte e2Robot2) && e2Robot2 == 0x18,
                "episode 2 Trashcan robot must map to class 0x18.");
            Assert(OriginalGuardProfiles.TryClassFromMapObjectId(
                       2, 0xC8, out byte e2Cannon) && e2Cannon == 0x19,
                "episode 2 Cannon must map to class 0x19.");
            Assert(!OriginalGuardProfiles.TryClassFromMapObjectId(
                       2, 0xCC, out _),
                "episode 2 object 0xCC must not inherit episode 1 Cannon mapping.");

            Assert(OriginalGuardProfiles.TryClassFromMapObjectId(
                       3, 0x70, out byte e3Penelope) && e3Penelope == 0x15,
                "episode 3 Penelope must map to class 0x15.");
            Assert(OriginalGuardProfiles.TryClassFromMapObjectId(
                       3, 0x78, out byte e3Hamerstein) && e3Hamerstein == 0x16,
                "episode 3 Hamerstein must map to class 0x16.");
            Assert(OriginalGuardProfiles.TryClassFromMapObjectId(
                       3, 0x80, out byte e3Ghost) && e3Ghost == 0x1A,
                "episode 3 Ghost must map to class 0x1A.");
            Assert(OriginalGuardProfiles.TryClassFromMapObjectId(
                       3, 0x8C, out byte e3Demon) && e3Demon == 0x1D,
                "episode 3 Demon must map to class 0x1D.");
            Assert(OriginalGuardProfiles.TryClassFromMapObjectId(
                       3, 0x90, out byte e3Alien1) && e3Alien1 == 0x1E,
                "episode 3 Alien #1 must map to class 0x1E.");
            Assert(OriginalGuardProfiles.TryClassFromMapObjectId(
                       3, 0x98, out byte e3Alien2) && e3Alien2 == 0x1F,
                "episode 3 Alien #2 must map to class 0x1F.");

            Assert(OriginalGuardProfiles.TryClassFromMapObjectId(
                       0x80, out byte compatibilityBat) &&
                   compatibilityBat == 0x08,
                "episode-1 compatibility map-class overload mismatch.");
        }

        static void TestGuardInitialProfiles()
        {
            var generic = OriginalGuardProfiles.ForObjectClass(0x0B);
            Assert(generic.Strategy == 0 &&
                   generic.State == (byte)OriginalGuardState.Active07 &&
                   generic.NextState == (byte)OriginalGuardState.Active02 &&
                   generic.PerceptionMode == 1,
                "generic guard initial profile mismatch.");

            var gargoyle = OriginalGuardProfiles.ForObjectClass(0x12);
            Assert(gargoyle.Strategy == 3 &&
                   gargoyle.State == (byte)OriginalGuardState.Active07 &&
                   gargoyle.NextState == (byte)OriginalGuardState.Active02 &&
                   gargoyle.PerceptionMode == 0,
                "gargoyle guard initial profile mismatch.");

            var cannon = OriginalGuardProfiles.ForObjectClass(0x19);
            Assert(cannon.Strategy == 4 &&
                   cannon.State == (byte)OriginalGuardState.Conditional0E,
                "cannon guard initial profile mismatch.");

            var spawned = new OriginalGuardRecord
            {
                Strategy = 0,
                State = (byte)OriginalGuardState.Active07
            };
            OriginalGuardProfiles.ApplySpawnVariantAndCell(
                ref spawned, 4, 0x42);
            Assert(spawned.Octant == 0 &&
                   spawned.MoveX == 0 &&
                   spawned.MoveY == -8 &&
                   spawned.State == (byte)OriginalGuardState.Move08 &&
                   spawned.Strategy == 2,
                "moving north variant / wall-class-42 spawn override mismatch.");

            spawned = new OriginalGuardRecord
            {
                Strategy = 0,
                State = (byte)OriginalGuardState.Active07
            };
            OriginalGuardProfiles.ApplySpawnVariantAndCell(
                ref spawned, 5, 0x43);
            Assert(spawned.Octant == 2 &&
                   spawned.MoveX == 8 &&
                   spawned.MoveY == 0 &&
                   spawned.State == (byte)OriginalGuardState.Move08 &&
                   spawned.Strategy == 1,
                "moving east variant / wall-class-43 spawn override mismatch.");

            OriginalGuardDispatcher.GetDirectionalStep(
                1, 2, out sbyte moveX, out sbyte moveY);
            Assert(moveX == 16 && moveY == 0,
                "strategy 2 must double normal directional movement to 16.");
        }

        static void TestState07Decision()
        {
            var guard = new OriginalGuardRecord
            {
                State = (byte)OriginalGuardState.Active07,
                Strategy = 0
            };

            Assert(OriginalGuardDispatcher.ResolveState07Perception(
                       ref guard, true, true, 0) ==
                   OriginalGuardDispatchResult.Waiting,
                "state 07 processing gate must suppress transition.");
            Assert(guard.State == (byte)OriginalGuardState.Active07,
                "state 07 must remain active when processing is gated.");

            Assert(OriginalGuardDispatcher.ResolveState07Perception(
                       ref guard, false, true, 0) ==
                   OriginalGuardDispatchResult.Transitioned,
                "state 07 successful perception should transition.");
            Assert(guard.State == (byte)OriginalGuardState.Active02,
                "ordinary state 07 perception must enter state 02.");

            guard.State = (byte)OriginalGuardState.Active07;
            guard.Strategy = 3;
            guard.Octant = 3;

            Assert(OriginalGuardDispatcher.ResolveState07Perception(
                       ref guard, false, true, 1) ==
                   OriginalGuardDispatchResult.Transitioned,
                "strategy-3 state 07 should enter timed movement.");
            Assert(guard.State == (byte)OriginalGuardState.Transition13 &&
                   guard.Timer == 9 &&
                   guard.MoveX == 0 &&
                   guard.MoveY == 8,
                "strategy-3 state 07 transition mismatch.");
        }

        static void TestState08WallTurn()
        {
            var guard = new OriginalGuardRecord
            {
                State = (byte)OriginalGuardState.Move08,
                Strategy = 0,
                Octant = 0
            };
            var obj = new OriginalObjectRecord
            {
                WorldX = 96,
                WorldY = 96
            };

            Assert(OriginalGuardDispatcher.ApplyState08WallTurn(
                       ref guard,
                       ref obj,
                       0x41,
                       2) == OriginalGuardDispatchResult.Transitioned &&
                   guard.Octant == 2 &&
                   guard.MoveX == 8 &&
                   guard.MoveY == 0,
                "state 08 class-41 variant must select facing/movement.");

            guard = new OriginalGuardRecord
            {
                State = (byte)OriginalGuardState.Move08,
                Strategy = 2,
                MoveX = 0,
                MoveY = 0
            };
            obj.WorldX = 96;
            obj.WorldY = 96;

            Assert(OriginalGuardDispatcher.ApplyState08WallTurn(
                       ref guard,
                       ref obj,
                       0x42,
                       8) == OriginalGuardDispatchResult.Transitioned &&
                   guard.State == (byte)OriginalGuardState.Detection03,
                "state 08 class-42 variant 8 must enter state 03.");

            guard = new OriginalGuardRecord
            {
                State = (byte)OriginalGuardState.Move08,
                Strategy = 0,
                Octant = 1,
                MoveX = 8,
                MoveY = 0
            };

            Assert(OriginalGuardDispatcher.ApplyState08WallTurn(
                       ref guard,
                       ref obj,
                       0x42,
                       3) == OriginalGuardDispatchResult.Transitioned &&
                   guard.State == (byte)OriginalGuardState.Detection03 &&
                   guard.MoveX == 0 &&
                   guard.MoveY == 0 &&
                   guard.Octant == 5,
                "active state-08 class-42 turn must stop, enter 03 and reverse facing.");

            guard = new OriginalGuardRecord
            {
                State = (byte)OriginalGuardState.Move08,
                Strategy = 0
            };
            obj.WorldX = 97;
            obj.WorldY = 96;

            Assert(OriginalGuardDispatcher.ApplyState08WallTurn(
                       ref guard,
                       ref obj,
                       0x41,
                       2) == OriginalGuardDispatchResult.Waiting,
                "state 08 wall-turn helper must only act at tile center.");
        }

        static void TestStates0E0F10()
        {
            var definition = new OriginalObjectDefinitionRecord
            {
                DirectionalA0 = 0x02A0,
                DirectionalA1 = 0x02A0,
                DirectionalA2 = 0x02A0,
                DirectionalA3 = 0x02A0,
                DirectionalA4 = 0x02A0,
                DirectionalA5 = 0x02A0,
                DirectionalA6 = 0x02A0,
                DirectionalA7 = 0x02A0,
                DirectionalB0 = 0x03B0,
                DirectionalB1 = 0x03B0,
                DirectionalB2 = 0x03B0,
                DirectionalB3 = 0x03B0,
                DirectionalB4 = 0x03B0,
                DirectionalB5 = 0x03B0,
                DirectionalB6 = 0x03B0,
                DirectionalB7 = 0x03B0
            };

            var guard = new OriginalGuardRecord
            {
                State = (byte)OriginalGuardState.Conditional0E,
                Control = 1,
                Octant = 2,
                ResultOctant = 0,
                Timer = 9
            };
            var obj = new OriginalObjectRecord
            {
                WorldX = 0,
                WorldY = 0
            };

            Assert(OriginalGuardDispatcher.TickState0E(
                       ref guard, ref obj, definition, 64, 0, false) !=
                   OriginalGuardDispatchResult.NotHandled &&
                   guard.State == (byte)OriginalGuardState.Conditional0E,
                "state 0E must remain 0E while 51A5 gate is clear.");

            Assert(OriginalGuardDispatcher.TickState0E(
                       ref guard, ref obj, definition, 64, 0, true) ==
                   OriginalGuardDispatchResult.Transitioned &&
                   guard.State == (byte)OriginalGuardState.Timed0F &&
                   guard.Timer == 0,
                "state 0E must enter 0F with timer zero when 51A5 is set.");

            guard.Timer = 2;
            Assert(OriginalGuardDispatcher.TickState0F(
                       ref guard, ref obj, definition, 64, 0, true, true,
                       out bool soundEarly) ==
                   OriginalGuardDispatchResult.Waiting &&
                   guard.Timer == 1 &&
                   !soundEarly,
                "state 0F post-decrement must wait while old timer is nonzero.");

            guard.Timer = 0;
            Assert(OriginalGuardDispatcher.TickState0F(
                       ref guard, ref obj, definition, 64, 0, true, true,
                       out bool soundDue) ==
                   OriginalGuardDispatchResult.Transitioned &&
                   soundDue &&
                   guard.State == (byte)OriginalGuardState.AnimationTimer &&
                   guard.NextState == (byte)OriginalGuardState.Timed10 &&
                   guard.Timer == 3 &&
                   guard.DefinitionValue == 0x03B0 &&
                   unchecked((byte)obj.Component03) == 0xB0,
                "state 0F expiry must schedule bank-B state-0 animation to 0x10.");

            Assert(OriginalGuardDispatcher.CompleteDeferredState(ref guard) ==
                   OriginalGuardDispatchResult.Completed &&
                   guard.State == (byte)OriginalGuardState.Timed10,
                "state 0F animation must return through nextState 0x10.");

            guard.Timer = 1;
            Assert(!OriginalGuardDispatcher.State10AttackDue(
                       ref guard, ref obj, definition, 64, 0) &&
                   guard.Timer == 0,
                "state 0x10 old timer 1 must decrement to zero without attacking.");

            Assert(OriginalGuardDispatcher.State10AttackDue(
                       ref guard, ref obj, definition, 64, 0) &&
                   guard.Timer == -1,
                "state 0x10 old timer zero must decrement to -1 and attack.");

            Assert(OriginalGuardDispatcher.CompleteState10AttackCycle(
                       ref guard, ref obj, definition, 64, 0) ==
                   OriginalGuardDispatchResult.Transitioned &&
                   guard.State == (byte)OriginalGuardState.Timed0F &&
                   guard.Timer == 8 &&
                   guard.DefinitionValue == 0x02A0 &&
                   unchecked((byte)obj.Component03) == 0xA0,
                "state 0x10 tail must enter 0x0F timer 8 with bank-A sequence.");

            guard.State = (byte)OriginalGuardState.Timed0F;
            guard.Timer = 5;
            Assert(OriginalGuardDispatcher.TickState0F(
                       ref guard, ref obj, definition, 64, 0, false, true,
                       out bool gatedSound) ==
                   OriginalGuardDispatchResult.Transitioned &&
                   guard.State == (byte)OriginalGuardState.Conditional0E &&
                   !gatedSound,
                "state 0x0F must return to 0x0E when 51A5 is clear.");
        }

        static void TestState14DanceCycle()
        {
            var guard = new OriginalGuardRecord
            {
                State = (byte)OriginalGuardState.Periodic14,
                Timer = 0x70,
                Strategy = 2,
                NextState = 7
            };
            var obj = new OriginalObjectRecord
            {
                DefinitionId = 21
            };

            Assert(OriginalGuardDispatcher.TickState14Countdown(
                       ref guard,
                       out bool movement0,
                       out bool release0) ==
                   OriginalGuardDispatchResult.Waiting &&
                   guard.Timer == 0x6F &&
                   !movement0 &&
                   !release0,
                "state 14 initial 0x70 tick must wait without movement.");

            guard.Timer = 0x61;
            Assert(OriginalGuardDispatcher.TickState14Countdown(
                       ref guard,
                       out bool movement60,
                       out bool release60) ==
                   OriginalGuardDispatchResult.Waiting &&
                   guard.Timer == 0x60 &&
                   !movement60 &&
                   !release60,
                "state 14 timer 0x60 boundary must still wait.");

            Assert(OriginalGuardDispatcher.TickState14Countdown(
                       ref guard,
                       out bool movement5F,
                       out bool release5F) ==
                   OriginalGuardDispatchResult.Moved &&
                   guard.Timer == 0x5F &&
                   movement5F &&
                   !release5F,
                "state 14 timer below 0x60 must run FUN_71DC movement.");

            guard.Timer = 1;
            Assert(OriginalGuardDispatcher.TickState14Countdown(
                       ref guard,
                       out bool movementZero,
                       out bool releaseZero) ==
                   OriginalGuardDispatchResult.Moved &&
                   guard.Timer == 0 &&
                   movementZero &&
                   !releaseZero,
                "state 14 1->0 tick must still perform the final movement.");

            Assert(OriginalGuardDispatcher.TickState14Countdown(
                       ref guard,
                       out bool movementRelease,
                       out bool releaseGroup) ==
                   OriginalGuardDispatchResult.Transitioned &&
                   !movementRelease &&
                   releaseGroup,
                "state 14 must request global AE56(1) release only on next zero tick.");

            Assert(OriginalGuardDispatcher.ReleaseState14Record(
                       ref guard,
                       ref obj,
                       0x0333) &&
                   guard.Strategy == 0 &&
                   guard.State == (byte)OriginalGuardState.MoveThen03 &&
                   guard.Timer == 1 &&
                   obj.DefinitionId == 7 &&
                   guard.NextState == (byte)OriginalGuardState.MoveThen03 &&
                   guard.DefinitionValue == 0x0333,
                "AE56(1) state-14 record restoration mismatch.");
        }

        static void TestPainReturn()
        {
            var guard = new OriginalGuardRecord
            {
                State = (byte)OriginalGuardState.Pain15,
                NextState = (byte)OriginalGuardState.Active07
            };

            Assert(OriginalGuardDispatcher.CompletePainReaction(ref guard) ==
                   OriginalGuardDispatchResult.Completed,
                "pain reaction completion must be handled.");
            Assert(guard.State == (byte)OriginalGuardState.Active07,
                "state 15 must return through nextstate.");
        }

        static void TestDoorSelector()
        {
            byte[] wallIds = { 0x70, 0x77, 0x79, 0x80, 0x82, 0x83, 0xA3, 0xA6, 0xAD, 0xAE };
            foreach (byte wallId in wallIds)
            {
                Assert(OriginalDoorSelector.TryGet(wallId, out byte selector),
                    "known DOOR-family wall must produce selector.");
                Assert(selector == wallId - OriginalDoorSelector.FirstDoorWallId,
                    "DOOR selector arithmetic mismatch.");
            }

            Assert(!OriginalDoorSelector.TryGet(0x78, out _),
                "gap 0x78 must not be classified as DOOR family.");
            Assert(!OriginalDoorSelector.TryGet(0x81, out _),
                "gap 0x81 must not be classified as DOOR family.");
            Assert(!OriginalDoorSelector.TryGet(0xA7, out _),
                "wall 0xA7 must not be classified as DOOR family.");
        }

        static void TestWakeCache()
        {
            var cache = new OriginalGuardWakeCache();
            var guards = new OriginalGuardRecord[3];

            guards[0].Strategy = 0;
            guards[0].DefinitionLookup = 5;
            guards[0].State = (byte)OriginalGuardState.Active07;

            guards[1].Strategy = 0;
            guards[1].DefinitionLookup = 5;
            guards[1].State = (byte)OriginalGuardState.Move08;

            guards[2].Strategy = 1;
            guards[2].DefinitionLookup = 5;
            guards[2].State = (byte)OriginalGuardState.Active07;

            Assert(cache.Wake(0, guards, 3, i => 3) == 0,
                "wake selector zero must be a no-op.");

            int woke = cache.Wake(5, guards, 3, i => (ushort)(i + 3));
            Assert(woke == 2, "wake cache should wake exactly matching strategy-0 guards.");
            Assert(guards[0].State == (byte)OriginalGuardState.Delay &&
                   guards[0].Timer == 3,
                "first woken guard state/timer mismatch.");
            Assert(guards[1].State == (byte)OriginalGuardState.Delay &&
                   guards[1].Timer == 4,
                "second woken guard state/timer mismatch.");
            Assert(guards[2].State == (byte)OriginalGuardState.Active07,
                "nonzero-strategy guard must not be woken.");

            Assert(cache.Wake(5, guards, 3, i => 0) == 0,
                "wake selector must be one-shot until cache reset.");
        }
    }
}
