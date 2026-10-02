using System;
using System.Runtime.InteropServices;

namespace Nitemare3D
{
    /// <summary>
    /// Known byte-exact slice of the 0x5A-byte per-level object-definition block
    /// allocated by FUN_1010_4C8A. Unknown fields intentionally remain unnamed.
    /// </summary>
    [StructLayout(LayoutKind.Explicit, Pack = 1, Size = OriginalRuntime.ObjectDefinitionBytes)]
    public struct OriginalObjectDefinitionRecord
    {
        [FieldOffset(OriginalRuntime.ObjectDefinitionAlertSequenceOffset)]
        public ushort AlertSequence;

        [FieldOffset(OriginalRuntime.ObjectDefinitionAttackSequenceOffset)]
        public ushort AttackSequence;

        [FieldOffset(OriginalRuntime.ObjectDefinitionRecoverySequenceOffset)]
        public ushort RecoverySequence;
    }

    /// <summary>
    /// Mirrors the original per-level definition-index behavior without guessing
    /// the upstream resource resolver. The caller supplies the original-equivalent
    /// resource key and exact 0x5A-byte block. Equal source keys deduplicate to one
    /// byte-sized DefinitionId, matching OBJECT +0x04.
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
                    return true;

                case OriginalGuardState.Detection03:
                    packedSequence = definition.AttackSequence;
                    return true;

                case OriginalGuardState.DetectionAttack04:
                    packedSequence = definition.RecoverySequence;
                    return true;

                default:
                    packedSequence = 0;
                    return false;
            }
        }

        public bool TryGetOrAdd(
            uint sourceKey,
            byte[] block,
            out byte definitionId)
        {
            if (TryFindBySourceKey(sourceKey, out definitionId))
                return true;

            if (block == null ||
                block.Length < OriginalRuntime.ObjectDefinitionBytes ||
                Count >= OriginalRuntime.MaxObjectDefinitions)
            {
                definitionId = 0;
                return false;
            }

            definitionId = (byte)Count;
            sourceKeys[Count] = sourceKey;
            definitions[Count] = Parse(block, 0);
            Count++;
            return true;
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
                AlertSequence = ReadUInt16LittleEndian(
                    block,
                    offset + OriginalRuntime.ObjectDefinitionAlertSequenceOffset),
                AttackSequence = ReadUInt16LittleEndian(
                    block,
                    offset + OriginalRuntime.ObjectDefinitionAttackSequenceOffset),
                RecoverySequence = ReadUInt16LittleEndian(
                    block,
                    offset + OriginalRuntime.ObjectDefinitionRecoverySequenceOffset)
            };
        }

        static ushort ReadUInt16LittleEndian(byte[] data, int offset)
        {
            return (ushort)(data[offset] | (data[offset + 1] << 8));
        }
    }
}
