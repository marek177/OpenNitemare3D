using System;
using System.IO;
using Nitemare3D;

static class Program
{
    static int checks;
    static void Check(bool condition)
    { if (!condition) throw new Exception("Check failed: " + checks); ++checks; }
    static void Reject<T>(Action action) where T : Exception
    { try { action(); } catch (T) { ++checks; return; } throw new Exception("Expected " + typeof(T).Name); }

    static void Main()
    {
        var use = new RecoveredUseRuntime();
        Check(!use.Press(false));
        Check(use.Press(true));
        Check(!use.Press(true));
        Check(!use.Press(false));
        Check(use.Press(true));
        int[] offsets = { -64, 1, 1, 64, 64, -1, -1, -64 };
        for (int octant = 0; octant < 8; ++octant)
        {
            Check(RecoveredUseRuntime.TryTarget(20, 20, octant, out int tx, out int ty));
            Check(ty * 64 + tx == 20 * 64 + 20 + offsets[octant]);
        }
        Check(!RecoveredUseRuntime.TryTarget(63, 20, 2, out _, out _));
        Check(!RecoveredUseRuntime.TryTarget(0, 20, 6, out _, out _));
        Check(!RecoveredUseRuntime.TryTarget(20, 0, 0, out _, out _));
        Check(!RecoveredUseRuntime.TryTarget(20, 63, 4, out _, out _));
        Reject<ArgumentOutOfRangeException>(() => RecoveredUseRuntime.TryTarget(20, 20, 8, out _, out _));

        byte mask = 0;
        for (int i = 0; i < 4; ++i) mask = RecoveredInventory.GrantBit(mask, i);
        Check(mask == 15 && RecoveredInventory.HasAllPentagrams(mask));
        Check(RecoveredInventory.GrantBit(mask, 8) == mask);
        Check(!RecoveredInventory.HasBit(mask, -1));
        Check(RecoveredInventory.HasSecretPanelCredential(1));
        Check(!RecoveredInventory.HasSecretPanelCredential(2));
        Check(RecoveredInventory.CanUseKeyWarp(0x1C, 8));
        Check(!RecoveredInventory.CanUseKeyWarp(0x18, 255));

        var health = new RecoveredPlayerHealth { Value = 99 };
        Check(health.ApplyFixedPickup(30) && health.Value == 129);
        Check(health.ClampForHud() == 100);
        Check(!health.ApplyFixedPickup(20));
        Check(health.ApplyEnemyDamage(99) == EnemyDamageResult.NonLethal && health.Value == 1);
        Check(health.ApplyEnemyDamage(1) == EnemyDamageResult.Lethal && health.GameState == 2);
        health.RestoreTo100();
        Check(health.GameState == 2 && health.ApplyEnemyDamage(10) == EnemyDamageResult.AlreadyDead);
        health.GameState = 3;
        health.Omnipotent = true;
        Check(health.ApplyEnemyDamage(255) == EnemyDamageResult.Omnipotent);
        health.Omnipotent = false;
        Check(health.ApplyEnemyDamage(0) == EnemyDamageResult.NonLethal);

        var bytes = new byte[514 + 8192 * 2];
        bytes[0] = 2;
        bytes[514] = 42;
        bytes[514 + 8192 + 1] = 99;
        var map = new RecoveredMapArchive(bytes);
        Check(map.LevelCount == 2 && map.GetLevel(0)[0] == 42 && map.GetLevel(1)[1] == 99);
        bytes[514] = 0;
        Check(map.GetLevel(0)[0] == 42);
        Reject<ArgumentOutOfRangeException>(() => map.GetLevel(2));
        Reject<InvalidDataException>(() => new RecoveredMapArchive(new byte[513]));
        bytes[0] = 1;
        Reject<InvalidDataException>(() => new RecoveredMapArchive(bytes));

        var demo = new RecoveredDemoFile(new byte[] {
            10,0,5,0,20,0,
            0x1B,2,0,0,19,0,0,0,
            0x20,0,2,0xA5,19,0,0,0,
            0x0D,0x80,0,0,20,0,0,0 });
        Check(demo.HeaderWords[0] == 10 && demo.GenerationsAreMonotonic);
        int cursor = 0;
        Check(!demo.TryDispatch(18, ref cursor, out _) && cursor == 0);
        Check(demo.TryDispatch(19, ref cursor, out var first) && first.InputMask == 2 && cursor == 1);
        Check(demo.TryDispatch(19, ref cursor, out var second) && second.InputMask == 0x200 && second.PaddingByte == 0xA5);
        Check(!demo.TryDispatch(19, ref cursor, out _));
        Check(demo.TryDispatch(20, ref cursor, out _) && cursor == 3);
        Check(!demo.TryDispatch(uint.MaxValue, ref cursor, out _));
        Reject<InvalidDataException>(() => new RecoveredDemoFile(new byte[7]));
        var reverse = new RecoveredDemoFile(new byte[] {0,0,0,0,0,0, 0,0,0,0,2,0,0,0, 0,0,0,0,1,0,0,0});
        Check(!reverse.GenerationsAreMonotonic);
        Console.WriteLine(checks + " recovered C# checks passed.");
    }
}
