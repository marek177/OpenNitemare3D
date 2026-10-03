using System;
using System.Runtime.InteropServices;

namespace Nitemare3D
{
    /// <summary>
    /// Persistent 0x5A-byte IMG resource header copied by FUN_1010_4C8A.
    /// The per-definition metadata slot at DS:4748 points to this header.
    /// </summary>
    [StructLayout(LayoutKind.Explicit, Pack = 1, Size = OriginalRuntime.ObjectResourceHeaderBytes)]
    public struct OriginalObjectResourceHeader
    {
        [FieldOffset(0x02)] public byte FrameCount;

        [FieldOffset(OriginalRuntime.GuardState02ResourceWordOffset)]
        public ushort State02Word;

        [FieldOffset(OriginalRuntime.GuardState03ResourceWordOffset)]
        public ushort State03Word;

        [FieldOffset(OriginalRuntime.GuardState04ResourceWordOffset)]
        public ushort State04Word;
    }

    /// <summary>
    /// Runtime form of one 10-byte IMG frame entry produced by FUN_1010_4AB0.
    /// Only width/height survive from the raw 10-byte disk entry. +02 is replaced
    /// with the pixel-data file offset and +06 starts as a null cached-pixel pointer.
    /// </summary>
    [StructLayout(LayoutKind.Explicit, Pack = 1, Size = OriginalRuntime.ObjectResourceEntryBytes)]
    public struct OriginalImageFrameRuntimeRecord
    {
        [FieldOffset(0x00)] public byte Width;
        [FieldOffset(0x01)] public byte Height;
        [FieldOffset(0x02)] public uint PixelDataFileOffset;
        [FieldOffset(0x06)] public uint CachedPixelsPointer;
    }

    public sealed class OriginalObjectDefinitionCatalog
    {
        readonly uint[] sourceKeys =
            new uint[OriginalRuntime.MaxObjectDefinitions];
        readonly OriginalObjectResourceHeader[] headers =
            new OriginalObjectResourceHeader[OriginalRuntime.MaxObjectDefinitions];
        readonly byte[][] frameTables =
            new byte[OriginalRuntime.MaxObjectDefinitions][];

        public int Count { get; private set; }

        public void Clear()
        {
            Array.Clear(sourceKeys, 0, sourceKeys.Length);
            Array.Clear(headers, 0, headers.Length);
            Array.Clear(frameTables, 0, frameTables.Length);
            Count = 0;
        }

        public bool TryFindBySourceKey(uint sourceKey, out byte definitionId)
        {
            for (int i = 0; i < Count; i++)
            {
                if (sourceKeys[i] == sourceKey)
                {
                    definitionId = (byte)i;
                    return true;
                }
            }

            definitionId = 0;
            return false;
        }

        public bool TryGetHeader(
            byte definitionId,
            out OriginalObjectResourceHeader header)
        {
            if (definitionId < Count)
            {
                header = headers[definitionId];
                return true;
            }

            header = default;
            return false;
        }

        public bool TryGetFrameTable(byte definitionId, out byte[] table)
        {
            if (definitionId < Count && frameTables[definitionId] != null)
            {
                table = frameTables[definitionId];
                return true;
            }

            table = null;
            return false;
        }

        public bool TryGetGuardStateWord(
            byte definitionId,
            OriginalGuardState state,
            out ushort value)
        {
            value = 0;
            if (!TryGetHeader(definitionId, out var header))
                return false;

            switch (state)
            {
                case OriginalGuardState.Active02:
                    value = header.State02Word;
                    return true;

                case OriginalGuardState.Detection03:
                    value = header.State03Word;
                    return true;

                case OriginalGuardState.DetectionAttack04:
                    value = header.State04Word;
                    return true;

                default:
                    return false;
            }
        }

        public bool TryGetOrAdd(
            uint sourceKey,
            byte[] headerBytes,
            byte[] decodedFrameTable,
            out byte definitionId)
        {
            if (TryFindBySourceKey(sourceKey, out definitionId))
                return true;

            if (headerBytes == null ||
                headerBytes.Length < OriginalRuntime.ObjectResourceHeaderBytes ||
                decodedFrameTable == null ||
                decodedFrameTable.Length % OriginalRuntime.ObjectResourceEntryBytes != 0 ||
                Count >= OriginalRuntime.MaxObjectDefinitions)
            {
                definitionId = 0;
                return false;
            }

            var header = ParseHeader(headerBytes, 0);
            if (decodedFrameTable.Length !=
                header.FrameCount * OriginalRuntime.ObjectResourceEntryBytes)
            {
                definitionId = 0;
                return false;
            }

            definitionId = (byte)Count;
            sourceKeys[Count] = sourceKey;
            headers[Count] = header;
            frameTables[Count] = (byte[])decodedFrameTable.Clone();
            Count++;
            return true;
        }

        public static OriginalObjectResourceHeader ParseHeader(
            byte[] headerBytes,
            int offset)
        {
            if (headerBytes == null)
                throw new ArgumentNullException(nameof(headerBytes));
            if (offset < 0 ||
                offset > headerBytes.Length - OriginalRuntime.ObjectResourceHeaderBytes)
            {
                throw new ArgumentOutOfRangeException(nameof(offset));
            }

            return new OriginalObjectResourceHeader
            {
                FrameCount = headerBytes[offset + 0x02],
                State02Word = ReadUInt16LittleEndian(
                    headerBytes,
                    offset + OriginalRuntime.GuardState02ResourceWordOffset),
                State03Word = ReadUInt16LittleEndian(
                    headerBytes,
                    offset + OriginalRuntime.GuardState03ResourceWordOffset),
                State04Word = ReadUInt16LittleEndian(
                    headerBytes,
                    offset + OriginalRuntime.GuardState04ResourceWordOffset)
            };
        }

        public static int HeaderOffset(byte imageId, bool tileBank)
        {
            int bankedId = tileBank ? imageId : 0x100 + imageId;
            return 0x800 + bankedId * OriginalRuntime.ObjectResourceHeaderBytes;
        }

        static ushort ReadUInt16LittleEndian(byte[] data, int offset)
        {
            return (ushort)(data[offset] | (data[offset + 1] << 8));
        }
    }
}
