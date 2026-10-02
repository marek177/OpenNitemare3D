using System;
using System.Collections.Generic;
using System.IO;

namespace Nitemare3D
{
    // Win16 layout only. Header words remain raw until their meaning is verified.
    public readonly struct RecoveredDemoRecord
    {
        public readonly byte EventByte, PaddingByte;
        public readonly ushort InputMask;
        public readonly uint RenderGeneration;
        public RecoveredDemoRecord(byte key, ushort mask, byte padding, uint generation)
        { EventByte = key; InputMask = mask; PaddingByte = padding; RenderGeneration = generation; }
    }

    public sealed class RecoveredDemoFile
    {
        public IReadOnlyList<ushort> HeaderWords { get; }
        public IReadOnlyList<RecoveredDemoRecord> Records { get; }
        public bool GenerationsAreMonotonic { get; }

        static ushort U16(byte[] b, int p) => (ushort)(b[p] | (b[p + 1] << 8));
        static uint U32(byte[] b, int p) => (uint)(b[p] | (b[p + 1] << 8) |
                                                    (b[p + 2] << 16) | (b[p + 3] << 24));
        public RecoveredDemoFile(byte[] bytes)
        {
            if (bytes == null) throw new ArgumentNullException(nameof(bytes));
            if (bytes.Length < 6 || (bytes.Length - 6) % 8 != 0)
                throw new InvalidDataException("Invalid Win16 DEMO size/layout.");
            HeaderWords = Array.AsReadOnly(new[] { U16(bytes, 0), U16(bytes, 2), U16(bytes, 4) });
            var records = new List<RecoveredDemoRecord>();
            bool monotonic = true;
            uint previous = 0;
            for (int p = 6; p < bytes.Length; p += 8)
            {
                var record = new RecoveredDemoRecord(bytes[p], U16(bytes, p + 1), bytes[p + 3], U32(bytes, p + 4));
                if (records.Count > 0 && record.RenderGeneration < previous) monotonic = false;
                previous = record.RenderGeneration;
                records.Add(record);
            }
            Records = records.AsReadOnly();
            GenerationsAreMonotonic = monotonic;
        }

        // Original dispatch consumes at most one due event per call, including ties.
        public bool TryDispatch(uint generation, ref int cursor, out RecoveredDemoRecord record)
        {
            record = default;
            if (cursor < 0) throw new ArgumentOutOfRangeException(nameof(cursor));
            if (cursor >= Records.Count || Records[cursor].RenderGeneration > generation) return false;
            record = Records[cursor++];
            return true;
        }
    }
}
