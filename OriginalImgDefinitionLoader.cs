using System;

namespace Nitemare3D
{
    /// <summary>
    /// Byte-array equivalent of the recovered IMG resource path. This keeps the
    /// original per-level deduplication semantics while avoiding file I/O when the
    /// current Img instance already owns rawData.
    /// </summary>
    public static class OriginalImgDefinitionLoader
    {
        public static bool TryReadObjectDefinition(
            byte[] imgData,
            byte objectId,
            out uint sourceKey,
            out byte[] headerBytes,
            out byte[] runtimeFrameTable)
        {
            return TryReadResource(
                imgData,
                objectId,
                false,
                out sourceKey,
                out headerBytes,
                out runtimeFrameTable);
        }

        public static bool TryReadResource(
            byte[] imgData,
            byte imageId,
            bool tileBank,
            out uint sourceKey,
            out byte[] headerBytes,
            out byte[] runtimeFrameTable)
        {
            sourceKey = 0;
            headerBytes = null;
            runtimeFrameTable = null;

            if (imgData == null ||
                imgData.Length < OriginalRuntime.ImgFirstFrameStreamOffset)
            {
                return false;
            }

            // IMG uses two independent 0x400-byte directories:
            // wall/low-bank at 0x0000 and object/high-bank at 0x0400.
            int directoryOffset =
                (tileBank ? 0x0000 : 0x0400) +
                imageId * 4;
            if (directoryOffset >
                imgData.Length - sizeof(uint))
            {
                return false;
            }

            sourceKey = ReadUInt32LittleEndian(imgData, directoryOffset);
            if (sourceKey == 0 ||
                sourceKey < OriginalRuntime.ImgFirstFrameStreamOffset ||
                sourceKey >= imgData.Length)
            {
                return false;
            }

            int headerOffset =
                OriginalObjectDefinitionCatalog.HeaderOffset(imageId, tileBank);
            if (headerOffset < 0 ||
                headerOffset >
                    imgData.Length - OriginalRuntime.ObjectResourceHeaderBytes)
            {
                return false;
            }

            headerBytes = new byte[OriginalRuntime.ObjectResourceHeaderBytes];
            Array.Copy(
                imgData,
                headerOffset,
                headerBytes,
                0,
                headerBytes.Length);

            var header =
                OriginalObjectDefinitionCatalog.ParseHeader(headerBytes, 0);

            runtimeFrameTable =
                new byte[header.FrameCount * OriginalRuntime.ObjectResourceEntryBytes];

            long cursor = sourceKey;
            for (int i = 0; i < header.FrameCount; i++)
            {
                if (cursor >
                    imgData.Length - OriginalRuntime.ObjectResourceEntryBytes)
                {
                    return false;
                }

                int rawOffset = checked((int)cursor);
                byte width = imgData[rawOffset + 0];
                byte height = imgData[rawOffset + 1];

                if (tileBank)
                {
                    if (height != 0x40 ||
                        (width != 0x40 && width != 0x80))
                    {
                        return false;
                    }
                }
                else if (width * height > 0x0C00)
                {
                    return false;
                }

                uint pixelDataOffset =
                    checked((uint)(cursor + OriginalRuntime.ObjectResourceEntryBytes));
                long pixelBytes = (long)width * height;
                if (pixelDataOffset + pixelBytes > imgData.Length)
                    return false;

                int entryOffset =
                    i * OriginalRuntime.ObjectResourceEntryBytes;
                runtimeFrameTable[entryOffset + 0] = width;
                runtimeFrameTable[entryOffset + 1] = height;
                WriteUInt32LittleEndian(
                    runtimeFrameTable,
                    entryOffset + 2,
                    pixelDataOffset);
                WriteUInt32LittleEndian(
                    runtimeFrameTable,
                    entryOffset + 6,
                    0);

                cursor = pixelDataOffset + pixelBytes;
            }

            return true;
        }

        public static bool TryRegisterObjectDefinition(
            byte[] imgData,
            byte objectId,
            OriginalObjectDefinitionCatalog catalog,
            out byte definitionId)
        {
            definitionId = 0;

            if (catalog == null ||
                !TryReadObjectDefinition(
                    imgData,
                    objectId,
                    out uint sourceKey,
                    out byte[] headerBytes,
                    out byte[] runtimeFrameTable))
            {
                return false;
            }

            return catalog.TryGetOrAdd(
                sourceKey,
                headerBytes,
                runtimeFrameTable,
                out definitionId);
        }

        static uint ReadUInt32LittleEndian(byte[] data, int offset)
        {
            return (uint)(
                data[offset] |
                (data[offset + 1] << 8) |
                (data[offset + 2] << 16) |
                (data[offset + 3] << 24));
        }

        static void WriteUInt32LittleEndian(
            byte[] data,
            int offset,
            uint value)
        {
            data[offset + 0] = (byte)value;
            data[offset + 1] = (byte)(value >> 8);
            data[offset + 2] = (byte)(value >> 16);
            data[offset + 3] = (byte)(value >> 24);
        }
    }
}
