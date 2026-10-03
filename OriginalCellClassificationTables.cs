using System;
using System.IO;

namespace Nitemare3D
{
    /// <summary>
    /// Exact Win16 cell-classification tables loaded by FUN_1010_4868.
    ///
    /// FUN_4868 reads the 0x202-byte MAP.n header directly to DS:8194:
    ///   DS:8194      = uint16 level count
    ///   DS:8196[256] = raw wall-id -> semantic wall class
    ///   DS:8296[256] = raw object-id -> semantic object class
    ///
    /// FUN_24BC/FUN_2556 then derive DS:7E94/7F94 property bytes from these
    /// two classification tables. No EXE data-segment or runtime memory dump is
    /// required to reconstruct them.
    /// </summary>
    public sealed class OriginalCellClassificationTables
    {
        public const int TableBytes = 256;
        public const int CombinedTableBytes = TableBytes * 2;
        public const int MapHeaderBytes = 0x202;
        public const int MapWallClassOffset = 0x0002;
        public const int MapObjectClassOffset = 0x0102;

        readonly byte[] primaryMappedTypes = new byte[TableBytes];
        readonly byte[] secondaryMappedTypes = new byte[TableBytes];
        readonly byte[] primaryFlags = new byte[TableBytes];
        readonly byte[] secondaryFlags = new byte[TableBytes];

        public ushort LevelCount { get; private set; }
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

        public void LoadMapHeader(byte[] mapHeaderOrFile)
        {
            if (mapHeaderOrFile == null)
                throw new ArgumentNullException(nameof(mapHeaderOrFile));
            if (mapHeaderOrFile.Length < MapHeaderBytes)
                throw new InvalidDataException("MAP data is shorter than the 0x202-byte header.");

            LevelCount = (ushort)(
                mapHeaderOrFile[0] |
                (mapHeaderOrFile[1] << 8));

            Array.Copy(
                mapHeaderOrFile,
                MapWallClassOffset,
                primaryMappedTypes,
                0,
                TableBytes);
            Array.Copy(
                mapHeaderOrFile,
                MapObjectClassOffset,
                secondaryMappedTypes,
                0,
                TableBytes);

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

            LevelCount = 0;
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
