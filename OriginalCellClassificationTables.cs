using System;
using System.IO;

namespace Nitemare3D
{
    /// <summary>
    /// Exact runtime classification tables used by the original Win16 engine.
    ///
    /// DS:8196[256] maps first MAP-cell bytes to wall/runtime semantic classes.
    /// DS:8296[256] maps second MAP-cell bytes to object/runtime semantic classes.
    ///
    /// Current static exports expose these addresses as zero-filled, while all
    /// gameplay code reads them as populated tables. This class intentionally
    /// separates acquisition of the 512 bytes from the now-closed property and
    /// LOS semantics that consume them.
    /// </summary>
    public sealed class OriginalCellClassificationTables
    {
        public const int TableBytes = 256;
        public const int CombinedDumpBytes = TableBytes * 2;

        readonly byte[] primaryMappedTypes = new byte[TableBytes];
        readonly byte[] secondaryMappedTypes = new byte[TableBytes];
        readonly byte[] primaryFlags = new byte[TableBytes];
        readonly byte[] secondaryFlags = new byte[TableBytes];

        public bool IsLoaded { get; private set; }

        public byte PrimaryMappedType(byte id)
        {
            EnsureLoaded();
            return primaryMappedTypes[id];
        }

        public byte SecondaryMappedType(byte id)
        {
            EnsureLoaded();
            return secondaryMappedTypes[id];
        }

        public byte PrimaryFlags(byte id)
        {
            EnsureLoaded();
            return primaryFlags[id];
        }

        public byte SecondaryFlags(byte id)
        {
            EnsureLoaded();
            return secondaryFlags[id];
        }

        public void LoadCombinedDump(byte[] dump)
        {
            if (dump == null)
                throw new ArgumentNullException(nameof(dump));
            if (dump.Length != CombinedDumpBytes)
                throw new InvalidDataException(
                    "Expected exactly 512 bytes: DS:8196[256] followed by DS:8296[256].");

            Array.Copy(dump, 0, primaryMappedTypes, 0, TableBytes);
            Array.Copy(dump, TableBytes, secondaryMappedTypes, 0, TableBytes);

            RebuildFlags();
            IsLoaded = true;
        }

        public void LoadSeparate(byte[] primary, byte[] secondary)
        {
            if (primary == null)
                throw new ArgumentNullException(nameof(primary));
            if (secondary == null)
                throw new ArgumentNullException(nameof(secondary));
            if (primary.Length != TableBytes || secondary.Length != TableBytes)
                throw new InvalidDataException("Each classification table must be exactly 256 bytes.");

            Array.Copy(primary, primaryMappedTypes, TableBytes);
            Array.Copy(secondary, secondaryMappedTypes, TableBytes);

            RebuildFlags();
            IsLoaded = true;
        }

        public bool TryGetLosCellInputs(
            byte primaryId,
            byte secondaryId,
            out byte mappedPrimary,
            out byte mappedSecondary,
            out byte generatedPrimaryFlags,
            out byte generatedSecondaryFlags)
        {
            if (!IsLoaded)
            {
                mappedPrimary = 0;
                mappedSecondary = 0;
                generatedPrimaryFlags = 0;
                generatedSecondaryFlags = 0;
                return false;
            }

            mappedPrimary = primaryMappedTypes[primaryId];
            mappedSecondary = secondaryMappedTypes[secondaryId];
            generatedPrimaryFlags = primaryFlags[primaryId];
            generatedSecondaryFlags = secondaryFlags[secondaryId];
            return true;
        }

        public byte[] ExportCombinedDump()
        {
            EnsureLoaded();
            byte[] result = new byte[CombinedDumpBytes];
            Array.Copy(primaryMappedTypes, 0, result, 0, TableBytes);
            Array.Copy(secondaryMappedTypes, 0, result, TableBytes, TableBytes);
            return result;
        }

        void RebuildFlags()
        {
            for (int i = 0; i < TableBytes; i++)
            {
                primaryFlags[i] =
                    OriginalGuardDispatcher.BuildPrimaryCellFlags(primaryMappedTypes[i]);
                secondaryFlags[i] =
                    OriginalGuardDispatcher.BuildSecondaryCellFlags(secondaryMappedTypes[i]);
            }
        }

        void EnsureLoaded()
        {
            if (!IsLoaded)
                throw new InvalidOperationException("Classification tables have not been loaded.");
        }
    }
}
