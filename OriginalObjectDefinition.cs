using System;
using System.Runtime.InteropServices;

namespace Nitemare3D
{
    /// <summary>
    /// Byte-exact known tail of the 0x5A-byte IMG definition record used by
    /// OBJECT +0x04. FUN_1010_4B86 selects the object bank by adding 0x100
    /// before multiplying by 0x5A.
    /// </summary>
    [StructLayout(LayoutKind.Explicit, Pack = 1, Size = OriginalRuntime.ObjectDefinitionBytes)]
    public struct OriginalObjectDefinitionRecord
    {
        [FieldOffset(0x02)] public byte FrameCount;

        [FieldOffset(0x04)] public ulong DirectionalA0To3;
        [FieldOffset(0x0C)] public ulong DirectionalA4To7;
        [FieldOffset(0x14)] public ulong DirectionalB0To3;
        [FieldOffset(0x1C)] public ulong DirectionalB4To7;
        [FieldOffset(0x24)] public ulong DirectionalC0To3;
        [FieldOffset(0x2C)] public ulong DirectionalC4To7;

        [FieldOffset(OriginalRuntime.ObjectDefinitionAlertSequenceOffset)]
        public ushort AlertSequence;

        [FieldOffset(OriginalRuntime.ObjectDefinitionAttackSequenceOffset)]
        public ushort AttackSequence;

        [FieldOffset(OriginalRuntime.ObjectDefinitionRecoverySequenceOffset)]
        public ushort RecoverySequence;

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

        static ushort GetPackedDirectional(
            ulong lowFour,
            ulong highFour,
            int selector)
        {
            if (selector < 0 || selector > 7)
                return 0;

            ulong packed = selector < 4 ? lowFour : highFour;
            int shift = (selector & 3) * 16;
            return (ushort)(packed >> shift);
        }

        public ushort GetDirectionalSequenceA(int selector)
        {
            return GetPackedDirectional(
                DirectionalA0To3,
                DirectionalA4To7,
                selector);
        }

        public ushort GetDirectionalSequenceB(int selector)
        {
            return GetPackedDirectional(
                DirectionalB0To3,
                DirectionalB4To7,
                selector);
        }

        public ushort GetDirectionalSequenceC(int selector)
        {
            return GetPackedDirectional(
                DirectionalC0To3,
                DirectionalC4To7,
                selector);
        }

        public bool TryGetReactionSequence(int selector, out ushort sequence)
        {
            switch (selector)
            {
                case 0: sequence = ReactionSequence0; return (sequence & 0xFF00) != 0;
                case 1: sequence = ReactionSequence1; return (sequence & 0xFF00) != 0;
                case 2: sequence = ReactionSequence2; return (sequence & 0xFF00) != 0;
                case 3: sequence = ReactionSequence3; return (sequence & 0xFF00) != 0;
                case 4: sequence = ReactionSequence4; return (sequence & 0xFF00) != 0;
                case 5: sequence = ReactionSequence5; return (sequence & 0xFF00) != 0;
                case 6: sequence = ReactionSequence6; return (sequence & 0xFF00) != 0;
                case 7: sequence = ReactionSequence7; return (sequence & 0xFF00) != 0;
                default:
                    sequence = 0;
                    return false;
            }
        }

        public bool TryGetDeathSequence(int selector, out ushort sequence)
        {
            switch (selector)
            {
                case 0: sequence = DeathSequence0; return (sequence & 0xFF00) != 0;
                case 1: sequence = DeathSequence1; return (sequence & 0xFF00) != 0;
                case 2: sequence = DeathSequence2; return (sequence & 0xFF00) != 0;
                case 3: sequence = DeathSequence3; return (sequence & 0xFF00) != 0;
                case 4: sequence = DeathSequence4; return (sequence & 0xFF00) != 0;
                case 5: sequence = DeathSequence5; return (sequence & 0xFF00) != 0;
                case 6: sequence = DeathSequence6; return (sequence & 0xFF00) != 0;
                case 7: sequence = DeathSequence7; return (sequence & 0xFF00) != 0;
                default:
                    sequence = 0;
                    return false;
            }
        }
    }

    /// <summary>
    /// Mirrors FUN_1010_4C8A's per-level definition deduplication. Equal source
    /// keys collapse to one byte-sized DefinitionId, matching OBJECT +0x04.
    /// </summary>
    public sealed class OriginalObjectDefinitionCatalog
    {
        readonly uint[] sourceKeys =
            new uint[OriginalRuntime.MaxObjectDefinitions];
        readonly OriginalObjectDefinitionRecord[] definitions =
            new OriginalObjectDefinitionRecord[OriginalRuntime.MaxObjectDefinitions];

        public int Count { get; private set; }

        public void Clear()
        {
            Array.Clear(sourceKeys, 0, sourceKeys.Length);
            Array.Clear(definitions, 0, definitions.Length);
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

        public bool TryGet(byte definitionId, out OriginalObjectDefinitionRecord definition)
        {
            if (definitionId < Count)
            {
                definition = definitions[definitionId];
                return true;
            }

            definition = default;
            return false;
        }

        public bool TryGetSequence(
            byte definitionId,
            OriginalGuardState state,
            out ushort packedSequence)
        {
            if (!TryGet(definitionId, out var definition))
            {
                packedSequence = 0;
                return false;
            }

            switch (state)
            {
                case OriginalGuardState.Active02:
                    packedSequence = definition.AlertSequence;
                    return packedSequence != 0;

                case OriginalGuardState.Detection03:
                    packedSequence = definition.AttackSequence;
                    return packedSequence != 0;

                case OriginalGuardState.DetectionAttack04:
                    packedSequence = definition.RecoverySequence;
                    return packedSequence != 0;

                default:
                    packedSequence = 0;
                    return false;
            }
        }

        public bool TryGetReactionSequence(
            byte definitionId,
            int selector,
            out ushort packedSequence)
        {
            if (TryGet(definitionId, out var definition))
                return definition.TryGetReactionSequence(selector, out packedSequence);

            packedSequence = 0;
            return false;
        }

        public bool TryGetDeathSequence(
            byte definitionId,
            int selector,
            out ushort packedSequence)
        {
            if (TryGet(definitionId, out var definition))
                return definition.TryGetDeathSequence(selector, out packedSequence);

            packedSequence = 0;
            return false;
        }

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
            definitions[Count] = definition;
            Count++;
            return true;
        }

        public bool TryGetOrAdd(
            uint sourceKey,
            byte[] block,
            out byte definitionId)
        {
            if (TryFindBySourceKey(sourceKey, out definitionId))
                return true;

            if (block == null ||
                block.Length < OriginalRuntime.ObjectDefinitionBytes)
            {
                definitionId = 0;
                return false;
            }

            return TryGetOrAdd(sourceKey, Parse(block, 0), out definitionId);
        }

        public static OriginalObjectDefinitionRecord Parse(byte[] block, int offset)
        {
            if (block == null)
                throw new ArgumentNullException(nameof(block));
            if (offset < 0 ||
                offset > block.Length - OriginalRuntime.ObjectDefinitionBytes)
            {
                throw new ArgumentOutOfRangeException(nameof(offset));
            }

            return new OriginalObjectDefinitionRecord
            {
                FrameCount = block[offset + 0x02],
                DirectionalA0To3 = ReadUInt64LittleEndian(block, offset + 0x04),
                DirectionalA4To7 = ReadUInt64LittleEndian(block, offset + 0x0C),
                DirectionalB0To3 = ReadUInt64LittleEndian(block, offset + 0x14),
                DirectionalB4To7 = ReadUInt64LittleEndian(block, offset + 0x1C),
                DirectionalC0To3 = ReadUInt64LittleEndian(block, offset + 0x24),
                DirectionalC4To7 = ReadUInt64LittleEndian(block, offset + 0x2C),
                AlertSequence = ReadUInt16LittleEndian(block, offset + 0x34),
                AttackSequence = ReadUInt16LittleEndian(block, offset + 0x36),
                RecoverySequence = ReadUInt16LittleEndian(block, offset + 0x38),
                ReactionSequence0 = ReadUInt16LittleEndian(block, offset + 0x3A),
                ReactionSequence1 = ReadUInt16LittleEndian(block, offset + 0x3C),
                ReactionSequence2 = ReadUInt16LittleEndian(block, offset + 0x3E),
                ReactionSequence3 = ReadUInt16LittleEndian(block, offset + 0x40),
                ReactionSequence4 = ReadUInt16LittleEndian(block, offset + 0x42),
                ReactionSequence5 = ReadUInt16LittleEndian(block, offset + 0x44),
                ReactionSequence6 = ReadUInt16LittleEndian(block, offset + 0x46),
                ReactionSequence7 = ReadUInt16LittleEndian(block, offset + 0x48),
                DeathSequence0 = ReadUInt16LittleEndian(block, offset + 0x4A),
                DeathSequence1 = ReadUInt16LittleEndian(block, offset + 0x4C),
                DeathSequence2 = ReadUInt16LittleEndian(block, offset + 0x4E),
                DeathSequence3 = ReadUInt16LittleEndian(block, offset + 0x50),
                DeathSequence4 = ReadUInt16LittleEndian(block, offset + 0x52),
                DeathSequence5 = ReadUInt16LittleEndian(block, offset + 0x54),
                DeathSequence6 = ReadUInt16LittleEndian(block, offset + 0x56),
                DeathSequence7 = ReadUInt16LittleEndian(block, offset + 0x58)
            };
        }

        static ushort ReadUInt16LittleEndian(byte[] data, int offset)
        {
            return (ushort)(data[offset] | (data[offset + 1] << 8));
        }

        static ulong ReadUInt64LittleEndian(byte[] data, int offset)
        {
            ulong value = 0;
            for (int i = 0; i < 8; i++)
                value |= ((ulong)data[offset + i]) << (i * 8);
            return value;
        }
    }
}
