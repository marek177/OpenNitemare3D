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
            TestDelayState();
            TestState13Movement();
            TestState07Decision();
            TestPainReturn();
            TestWakeCache();

            Console.WriteLine("OpenNitemare3D runtime self-test: PASS");
        }

        static void TestRecordSizes()
        {
            Assert(Marshal.SizeOf<OriginalObjectRecord>() == 28,
                "OBJECT record must be 28 bytes.");
            Assert(Marshal.SizeOf<OriginalGuardRecord>() == 26,
                "GUARD record must be 26 bytes.");
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

            Assert(moved == 7, "state 13 must perform seven movement updates.");
            Assert(obj.WorldX == 156 && obj.WorldY == 100,
                "state 13 total displacement must be 56 world units.");
            Assert(guard.Strategy == 0 &&
                   guard.State == (byte)OriginalGuardState.Active02,
                "state 13 must clear strategy and return to state 02.");
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
