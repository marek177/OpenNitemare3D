using System;
using System.Runtime.InteropServices;

namespace Nitemare3D
{
    /// <summary>
    /// Byte-exact 0x5A IMG definition header copied by FUN_1010_4C8A.
    /// The per-definition metadata slot at DS:4748 points to this record.
    /// </summary>
    [StructLayout(LayoutKind.Explicit, Pack = 1, Size = OriginalRuntime.ObjectResourceHeaderBytes)]
    public struct OriginalObjectDefinitionRecord
    {
        [FieldOffset(0x02)] public byte FrameCount;

        [FieldOffset(0x04)] public ushort DirectionalA0;
        [FieldOffset(0x06)] public ushort DirectionalA1;
        [FieldOffset(0x08)] public ushort DirectionalA2;
        [FieldOffset(0x0A)] public ushort DirectionalA3;
        [FieldOffset(0x0C)] public ushort DirectionalA4;
        [FieldOffset(0x0E)] public ushort DirectionalA5;
        [FieldOffset(0x10)] public ushort DirectionalA6;
        [FieldOffset(0x12)] public ushort DirectionalA7;

        [FieldOffset(0x14)] public ushort DirectionalB0;
        [FieldOffset(0x16)] public ushort DirectionalB1;
        [FieldOffset(0x18)] public ushort DirectionalB2;
        [FieldOffset(0x1A)] public ushort DirectionalB3;
        [FieldOffset(0x1C)] public ushort DirectionalB4;
        [FieldOffset(0x1E)] public ushort DirectionalB5;
        [FieldOffset(0x20)] public ushort DirectionalB6;
        [FieldOffset(0x22)] public ushort DirectionalB7;

        [FieldOffset(0x24)] public ushort DirectionalC0;
        [FieldOffset(0x26)] public ushort DirectionalC1;
        [FieldOffset(0x28)] public ushort DirectionalC2;
        [FieldOffset(0x2A)] public ushort DirectionalC3;
        [FieldOffset(0x2C)] public ushort DirectionalC4;
        [FieldOffset(0x2E)] public ushort DirectionalC5;
        [FieldOffset(0x30)] public ushort DirectionalC6;
        [FieldOffset(0x32)] public ushort DirectionalC7;

        [FieldOffset(0x34)] public ushort State02Sequence;
        [FieldOffset(0x36)] public ushort State03Sequence;
        [FieldOffset(0x38)] public ushort State04Sequence;

        [FieldOffset(0x3A)] public ushort ReactionSequence0;
        [FieldOffset(0x3C)] public ushort ReactionSequence1;
        [FieldOffset(0x3E)] public ushort ReactionSequence2;
        [FieldOffset(0x40)] public ushort ReactionSequence3;
        [FieldOffset(0x42)] public ushort ReactionSequence4;
        [FieldOffset(0x44)] public ushort ReactionSequence5;
        [FieldOffset(0x46)] public ushort ReactionSequence6;
        [FieldOffset(0x48)] public ushort ReactionSequence7;

        [FieldOffset(0x4A)] public ushort DeathSequence0;
        [FieldOffset(0x4C)] public ushort DeathSequence1;
        [FieldOffset(0x4E)] public ushort DeathSequence2;
        [FieldOffset(0x50)] public ushort DeathSequence3;
        [FieldOffset(0x52)] public ushort DeathSequence4;
        [FieldOffset(0x54)] public ushort DeathSequence5;
        [FieldOffset(0x56)] public ushort DeathSequence6;
        [FieldOffset(0x58)] public ushort DeathSequence7;

        public ushort GetDirectionalSequenceA(int selector)
        {
            switch (selector & 7)
            {
                case 0: return DirectionalA0;
                case 1: return DirectionalA1;
                case 2: return DirectionalA2;
                case 3: return DirectionalA3;
                case 4: return DirectionalA4;
                case 5: return DirectionalA5;
                case 6: return DirectionalA6;
                default: return DirectionalA7;
            }
        }

        public ushort GetDirectionalSequenceB(int selector)
        {
            switch (selector & 7)
            {
                case 0: return DirectionalB0;
                case 1: return DirectionalB1;
                case 2: return DirectionalB2;
                case 3: return DirectionalB3;
                case 4: return DirectionalB4;
                case 5: return DirectionalB5;
                case 6: return DirectionalB6;
                default: return DirectionalB7;
            }
        }

        public ushort GetDirectionalSequenceC(int selector)
        {
            switch (selector & 7)
            {
                case 0: return DirectionalC0;
                case 1: return DirectionalC1;
                case 2: return DirectionalC2;
                case 3: return DirectionalC3;
                case 4: return DirectionalC4;
                case 5: return DirectionalC5;
                case 6: return DirectionalC6;
                default: return DirectionalC7;
            }
        }

        public bool TryGetReactionSequence(int selector, out ushort sequence)
        {
            switch (selector & 7)
            {
                case 0: sequence = ReactionSequence0; break;
                case 1: sequence = ReactionSequence1; break;
                case 2: sequence = ReactionSequence2; break;
                case 3: sequence = ReactionSequence3; break;
                case 4: sequence = ReactionSequence4; break;
                case 5: sequence = ReactionSequence5; break;
                case 6: sequence = ReactionSequence6; break;
                default: sequence = ReactionSequence7; break;
            }

            return (sequence & 0xFF00) != 0;
        }

        public bool TryGetDeathSequence(int selector, out ushort sequence)
        {
            switch (selector & 7)
            {
                case 0: sequence = DeathSequence0; break;
                case 1: sequence = DeathSequence1; break;
                case 2: sequence = DeathSequence2; break;
                case 3: sequence = DeathSequence3; break;
                case 4: sequence = DeathSequence4; break;
                case 5: sequence = DeathSequence5; break;
                case 6: sequence = DeathSequence6; break;
                default: sequence = DeathSequence7; break;
            }

            return (sequence & 0xFF00) != 0;
        }
    }

    /// <summary>
    /// Runtime form of one 10-byte IMG frame entry produced by FUN_1010_4AB0.
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
        readonly OriginalObjectDefinitionRecord[] headers =
            new OriginalObjectDefinitionRecord[OriginalRuntime.MaxObjectDefinitions];
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
            out OriginalObjectDefinitionRecord header)
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
                    value = header.State02Sequence;
                    return true;
                case OriginalGuardState.Detection03:
                    value = header.State03Sequence;
                    return true;
                case OriginalGuardState.DetectionAttack04:
                    value = header.State04Sequence;
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

            var header = Parse(headerBytes, 0);
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

        // Header-only overload used by isolated GUARD logic tests.
        public bool TryGetOrAdd(
            uint sourceKey,
            OriginalObjectDefinitionRecord definition,
            out byte definitionId)
        {
            if (TryFindBySourceKey(sourceKey, out definitionId))
                return true;
            if (Count >= OriginalRuntime.MaxObjectDefinitions)
            {
                definitionId = 0;
                return false;
            }

            definitionId = (byte)Count;
            sourceKeys[Count] = sourceKey;
            headers[Count] = definition;
            frameTables[Count] = null;
            Count++;
            return true;
        }

        public static OriginalObjectDefinitionRecord Parse(
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

            var d = new OriginalObjectDefinitionRecord
            {
                FrameCount = headerBytes[offset + 0x02],
                State02Sequence = ReadUInt16LittleEndian(headerBytes, offset + 0x34),
                State03Sequence = ReadUInt16LittleEndian(headerBytes, offset + 0x36),
                State04Sequence = ReadUInt16LittleEndian(headerBytes, offset + 0x38)
            };

            d.DirectionalA0 = ReadUInt16LittleEndian(headerBytes, offset + 0x04);
            d.DirectionalA1 = ReadUInt16LittleEndian(headerBytes, offset + 0x06);
            d.DirectionalA2 = ReadUInt16LittleEndian(headerBytes, offset + 0x08);
            d.DirectionalA3 = ReadUInt16LittleEndian(headerBytes, offset + 0x0A);
            d.DirectionalA4 = ReadUInt16LittleEndian(headerBytes, offset + 0x0C);
            d.DirectionalA5 = ReadUInt16LittleEndian(headerBytes, offset + 0x0E);
            d.DirectionalA6 = ReadUInt16LittleEndian(headerBytes, offset + 0x10);
            d.DirectionalA7 = ReadUInt16LittleEndian(headerBytes, offset + 0x12);

            d.DirectionalB0 = ReadUInt16LittleEndian(headerBytes, offset + 0x14);
            d.DirectionalB1 = ReadUInt16LittleEndian(headerBytes, offset + 0x16);
            d.DirectionalB2 = ReadUInt16LittleEndian(headerBytes, offset + 0x18);
            d.DirectionalB3 = ReadUInt16LittleEndian(headerBytes, offset + 0x1A);
            d.DirectionalB4 = ReadUInt16LittleEndian(headerBytes, offset + 0x1C);
            d.DirectionalB5 = ReadUInt16LittleEndian(headerBytes, offset + 0x1E);
            d.DirectionalB6 = ReadUInt16LittleEndian(headerBytes, offset + 0x20);
            d.DirectionalB7 = ReadUInt16LittleEndian(headerBytes, offset + 0x22);

            d.DirectionalC0 = ReadUInt16LittleEndian(headerBytes, offset + 0x24);
            d.DirectionalC1 = ReadUInt16LittleEndian(headerBytes, offset + 0x26);
            d.DirectionalC2 = ReadUInt16LittleEndian(headerBytes, offset + 0x28);
            d.DirectionalC3 = ReadUInt16LittleEndian(headerBytes, offset + 0x2A);
            d.DirectionalC4 = ReadUInt16LittleEndian(headerBytes, offset + 0x2C);
            d.DirectionalC5 = ReadUInt16LittleEndian(headerBytes, offset + 0x2E);
            d.DirectionalC6 = ReadUInt16LittleEndian(headerBytes, offset + 0x30);
            d.DirectionalC7 = ReadUInt16LittleEndian(headerBytes, offset + 0x32);

            d.ReactionSequence0 = ReadUInt16LittleEndian(headerBytes, offset + 0x3A);
            d.ReactionSequence1 = ReadUInt16LittleEndian(headerBytes, offset + 0x3C);
            d.ReactionSequence2 = ReadUInt16LittleEndian(headerBytes, offset + 0x3E);
            d.ReactionSequence3 = ReadUInt16LittleEndian(headerBytes, offset + 0x40);
            d.ReactionSequence4 = ReadUInt16LittleEndian(headerBytes, offset + 0x42);
            d.ReactionSequence5 = ReadUInt16LittleEndian(headerBytes, offset + 0x44);
            d.ReactionSequence6 = ReadUInt16LittleEndian(headerBytes, offset + 0x46);
            d.ReactionSequence7 = ReadUInt16LittleEndian(headerBytes, offset + 0x48);

            d.DeathSequence0 = ReadUInt16LittleEndian(headerBytes, offset + 0x4A);
            d.DeathSequence1 = ReadUInt16LittleEndian(headerBytes, offset + 0x4C);
            d.DeathSequence2 = ReadUInt16LittleEndian(headerBytes, offset + 0x4E);
            d.DeathSequence3 = ReadUInt16LittleEndian(headerBytes, offset + 0x50);
            d.DeathSequence4 = ReadUInt16LittleEndian(headerBytes, offset + 0x52);
            d.DeathSequence5 = ReadUInt16LittleEndian(headerBytes, offset + 0x54);
            d.DeathSequence6 = ReadUInt16LittleEndian(headerBytes, offset + 0x56);
            d.DeathSequence7 = ReadUInt16LittleEndian(headerBytes, offset + 0x58);

            return d;
        }

        public static OriginalObjectDefinitionRecord ParseHeader(
            byte[] headerBytes,
            int offset)
        {
            return Parse(headerBytes, offset);
        }

        public static int HeaderOffset(byte imageId, bool tileBank)
        {
            int bankedId = tileBank ? imageId : 0x100 + imageId;
            return OriginalRuntime.ImgResourceHeadersOffset +
                   bankedId * OriginalRuntime.ObjectResourceHeaderBytes;
        }

        static ushort ReadUInt16LittleEndian(byte[] data, int offset)
        {
            return (ushort)(data[offset] | (data[offset + 1] << 8));
        }
    }
}
