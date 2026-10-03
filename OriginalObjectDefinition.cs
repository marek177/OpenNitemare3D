using System;

namespace Nitemare3D
{
    /// <summary>
    /// Neutral model of the per-level object resource table produced by
    /// FUN_1010_4B86. The original first reads a 0x5A-byte header, then allocates
    /// count*10 bytes and fills 10-byte decoded entries. The returned table pointer,
    /// not the header buffer, is stored in the per-level metadata rooted at DS:4748.
    /// </summary>
    public sealed class OriginalObjectDefinitionCatalog
    {
        readonly uint[] sourceKeys =
            new uint[OriginalRuntime.MaxObjectDefinitions];
        readonly byte[][] resourceTables =
            new byte[OriginalRuntime.MaxObjectDefinitions][];

        public int Count { get; private set; }

        public void Clear()
        {
            Array.Clear(sourceKeys, 0, sourceKeys.Length);
            Array.Clear(resourceTables, 0, resourceTables.Length);
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

        public bool TryGetTable(byte definitionId, out byte[] table)
        {
            if (definitionId < Count && resourceTables[definitionId] != null)
            {
                table = resourceTables[definitionId];
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
            if (!TryGetTable(definitionId, out var table))
                return false;

            int offset;
            switch (state)
            {
                case OriginalGuardState.Active02:
                    offset = OriginalRuntime.GuardState02ResourceWordOffset;
                    break;

                case OriginalGuardState.Detection03:
                    offset = OriginalRuntime.GuardState03ResourceWordOffset;
                    break;

                case OriginalGuardState.DetectionAttack04:
                    offset = OriginalRuntime.GuardState04ResourceWordOffset;
                    break;

                default:
                    return false;
            }

            if (offset < 0 || offset + 1 >= table.Length)
                return false;

            value = ReadUInt16LittleEndian(table, offset);
            return true;
        }

        public bool TryGetOrAdd(
            uint sourceKey,
            byte[] decodedResourceTable,
            out byte definitionId)
        {
            if (TryFindBySourceKey(sourceKey, out definitionId))
                return true;

            if (decodedResourceTable == null ||
                decodedResourceTable.Length == 0 ||
                decodedResourceTable.Length % OriginalRuntime.ObjectResourceEntryBytes != 0 ||
                Count >= OriginalRuntime.MaxObjectDefinitions)
            {
                definitionId = 0;
                return false;
            }

            definitionId = (byte)Count;
            sourceKeys[Count] = sourceKey;
            resourceTables[Count] = (byte[])decodedResourceTable.Clone();
            Count++;
            return true;
        }

        public static int DecodeEntryCountFromHeader(byte[] header, int offset)
        {
            if (header == null)
                throw new ArgumentNullException(nameof(header));
            if (offset < 0 ||
                offset > header.Length - OriginalRuntime.ObjectResourceHeaderBytes)
            {
                throw new ArgumentOutOfRangeException(nameof(offset));
            }

            // FUN_1010_4B86 reads the count from header +2 and allocates count*10.
            return header[offset + 2];
        }

        static ushort ReadUInt16LittleEndian(byte[] data, int offset)
        {
            return (ushort)(data[offset] | (data[offset + 1] << 8));
        }
    }
}
