using System;

namespace Nitemare3D
{
    /// <summary>
    /// Recovered 64-byte per-level AREA wake gate used by the successful
    /// player-fire/noise path. The selector is the persistent class-0x44 AREA id
    /// stored in DAT_4C1C for the player and GUARD+0x0E for each guard.
    /// </summary>
    public sealed class OriginalGuardWakeCache
    {
        public const int EntryCount = 64;

        readonly byte[] entries = new byte[EntryCount];

        public byte[] Entries => entries;

        public void Clear()
        {
            Array.Clear(entries, 0, entries.Length);
        }

        public bool IsSet(byte selector)
        {
            return selector < EntryCount && entries[selector] != 0;
        }

        /// <summary>
        /// Applies the confirmed one-shot AREA wake scan (FUN_1010_7664).
        /// selector 0 is an original no-op. Matching strategy-0 guards in states
        /// 7 or 8 receive RNG%8 as timer and enter state 1.
        /// </summary>
        public int Wake(
            byte selector,
            OriginalGuardRecord[] guards,
            int guardCount,
            Func<int, ushort> randomForGuard)
        {
            if (selector == 0 || selector >= EntryCount)
                return 0;

            if (entries[selector] != 0)
                return 0;

            // The original marks the selector before scanning the guard pool.
            entries[selector] = 1;

            int woke = 0;
            int count = Math.Min(guardCount, guards.Length);

            for (int i = 0; i < count; i++)
            {
                ref var guard = ref guards[i];

                if (guard.Strategy != 0 ||
                    guard.DefinitionLookup != selector ||
                    (guard.State != (byte)OriginalGuardState.Active07 &&
                     guard.State != (byte)OriginalGuardState.Move08))
                {
                    continue;
                }

                ushort random = randomForGuard != null
                    ? randomForGuard(i)
                    : (ushort)0;

                guard.Timer = (short)(random % 8);
                guard.State = (byte)OriginalGuardState.Delay;
                woke++;
            }

            return woke;
        }
    }
}
