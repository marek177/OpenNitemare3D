using System;
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
            TestObjectDefinitionCatalog();
            TestImgResourceLoader();
            TestOriginalMapTables();
            TestOriginalWallRuntime();
            TestDamageMatrix();
            TestGuardSounds();
            TestGuardToPlayerDamage();
            TestPackedGuardSequences();
            TestExactAnimationSchedulers();
            TestHitTransitionRouting();
            TestAttackGate();
            TestPerceptionPrefilter();
            TestGuardGridTrace();
            TestProjectileRuntime();
            TestDelayState();
            TestState13Movement();
            TestMovementPlanning();
            TestMovementCell700A();
            TestMovementCollisionCore();
            TestDirectionalSequenceRefresh();
            TestGuardMapClassMapping();
            TestGuardInitialProfiles();
            TestState07Decision();
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
            Assert(Marshal.SizeOf<OriginalProjectileRecord>() == 42,
                "projectile record must be 42 bytes.");
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
            header[0x02] = 6;
            header[0x34] = 0x12;
            header[0x35] = 0x04;
            header[0x36] = 0x20;
            header[0x37] = 0x03;
            header[0x38] = 0x30;
            header[0x39] = 0x02;

            var parsed = OriginalObjectDefinitionCatalog.ParseHeader(header, 0);
            Assert(parsed.FrameCount == 6 &&
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

            int directoryOffset = objectId * 4;
            img[directoryOffset + 0] = (byte)streamOffset;
            img[directoryOffset + 1] = (byte)(streamOffset >> 8);
            img[directoryOffset + 2] = (byte)(streamOffset >> 16);
            img[directoryOffset + 3] = (byte)(streamOffset >> 24);

            int headerOffset =
                OriginalObjectDefinitionCatalog.HeaderOffset(objectId, false);
            img[headerOffset + 0x02] = 1;
            img[headerOffset + 0x34] = 0x12;
            img[headerOffset + 0x35] = 0x04;
            img[headerOffset + 0x36] = 0x20;
            img[headerOffset + 0x37] = 0x03;
            img[headerOffset + 0x38] = 0x30;
            img[headerOffset + 0x39] = 0x02;

            img[streamOffset + 0] = 32;
            img[streamOffset + 1] = 32;

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
            Assert(definition.FrameCount == 1 &&
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
                   registered.State03Sequence == 0x0320,
                "synthetic IMG definition registration mismatch.");
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
                   door.TargetY == second.Y1,
                "FUN_14A8 paired-wall controller initialization mismatch.");

            Assert(walls.TogglePairedWall(1, 1, 2, false) &&
                   door.State == 2,
                "FUN_188A state-1 door must enter opening state 2.");

            walls.TickPairedWallMotion();
            Assert(door.State == 2,
                "paired-wall motion must remain opening before target is reached.");

            Assert(walls.ForceState(1, 1, 0) &&
                   door.State == 0,
                "paired-wall state 0 force mismatch.");

            Assert(walls.SetLatchedPassable(1, 1) &&
                   door.State == 4,
                "paired-wall state 4 latched/passable mismatch.");
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
