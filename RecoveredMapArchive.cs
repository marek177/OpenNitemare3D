using System;
using System.IO;

namespace Nitemare3D
{
    // Port of formats/MapArchive.cpp. MAP cells are row-major wall/object pairs.
    public sealed class RecoveredMapArchive
    {
        readonly byte[] bytes;
        public int LevelCount { get; }

        public RecoveredMapArchive(byte[] data)
        {
            if (data == null) throw new ArgumentNullException(nameof(data));
            if (data.Length < OriginalRuntime.MapHeaderBytes)
                throw new InvalidDataException("MAP header is truncated.");
            LevelCount = data[0] | (data[1] << 8);
            int payload = data.Length - OriginalRuntime.MapHeaderBytes;
            if (payload % OriginalRuntime.MapLevelBytes != 0 ||
                LevelCount != payload / OriginalRuntime.MapLevelBytes)
                throw new InvalidDataException("MAP level count does not match its payload.");
            bytes = (byte[])data.Clone();
        }

        public byte[] GetLevel(int index)
        {
            if (index < 0 || index >= LevelCount)
                throw new ArgumentOutOfRangeException(nameof(index));
            var level = new byte[OriginalRuntime.MapLevelBytes];
            Buffer.BlockCopy(bytes, OriginalRuntime.MapHeaderBytes + index * level.Length,
                             level, 0, level.Length);
            return level;
        }
    }
}
