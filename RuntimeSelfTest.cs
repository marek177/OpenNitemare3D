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
            TestDamageMatrix();
            TestGuardToPlayerDamage();
            TestPackedGuardSequences();
            TestAttackGate();
            TestProjectileRuntime();
            TestDelayState();
            TestState13Movement();
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

        static void TestGuardMapClassMapping()
        {
            Assert(OriginalGuardProfiles.TryClassFromMapObjectId(128, out byte bat) &&
                   bat == 0x08,
                "Bat MAP object must map to class 0x08.");
            Assert(OriginalGuardProfiles.TryClassFromMapObjectId(139, out byte mummy) &&
                   mummy == 0x0A,
                "Mummy MAP object must map to class 0x0A.");
            Assert(OriginalGuardProfiles.TryClassFromMapObjectId(144, out byte skeleton) &&
                   skeleton == 0x0B,
                "Skeleton MAP object must map to class 0x0B.");
            Assert(OriginalGuardProfiles.TryClassFromMapObjectId(160, out byte baddie1) &&
                   baddie1 == 0x0F,
                "Baddie #1 MAP object must map to class 0x0F.");
            Assert(OriginalGuardProfiles.TryClassFromMapObjectId(176, out byte dracula) &&
                   dracula == 0x11,
                "Dracula MAP object must map to class 0x11.");
            Assert(OriginalGuardProfiles.TryClassFromMapObjectId(180, out byte cemetery) &&
                   cemetery == 0x12,
                "Cemetery gargoyle MAP object must map to class 0x12.");
            Assert(OriginalGuardProfiles.TryClassFromMapObjectId(184, out byte garden) &&
                   garden == 0x13,
                "Garden gargoyle MAP object must map to class 0x13.");
            Assert(OriginalGuardProfiles.TryClassFromMapObjectId(188, out byte penelope) &&
                   penelope == 0x15,
                "Penelope MAP object must map to class 0x15.");
            Assert(OriginalGuardProfiles.TryClassFromMapObjectId(196, out byte hamerstein) &&
                   hamerstein == 0x16,
                "Hamerstein MAP object must map to class 0x16.");
            Assert(OriginalGuardProfiles.TryClassFromMapObjectId(204, out byte cannon) &&
                   cannon == 0x19,
                "Cannon MAP object must map to class 0x19.");
            Assert(!OriginalGuardProfiles.TryClassFromMapObjectId(140, out _),
                "Dancers must remain outside ordinary GUARD class mapping.");
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
