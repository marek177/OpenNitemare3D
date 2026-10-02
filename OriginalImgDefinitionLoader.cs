using System;

namespace Nitemare3D
{
    /// <summary>
    /// Clean-room parser for the two IMG definition banks recovered from
    /// FUN_1010_4B86. The object bank begins at 0x6200 and contains one 0x5A
    /// definition for each object id. The object directory at 0x0400 supplies
    /// the shared source/frame-stream key used by FUN_1010_4C8A deduplication.
    /// </summary>
    public static class OriginalImgDefinitionLoader
    {
        public static bool TryReadObjectDefinition(
            byte[] imgData,
            byte objectId,
            out uint sourceKey,
            out OriginalObjectDefinitionRecord definition)
        {
            sourceKey = 0;
            definition = default;

            if (imgData == null ||
                imgData.Length < OriginalRuntime.ImgFirstFrameStreamOffset)
            {
                return false;
            }

            int directoryOffset =
                OriginalRuntime.ImgObjectDirectoryOffset + objectId * 4;
            sourceKey = ReadUInt32LittleEndian(imgData, directoryOffset);

            // A zero directory pointer is the original "no resource" case.
            if (sourceKey == 0 ||
                sourceKey < OriginalRuntime.ImgFirstFrameStreamOffset ||
                sourceKey >= imgData.Length)
            {
                return false;
            }

            int definitionOffset =
                OriginalRuntime.ImgObjectDefinitionBankOffset +
                objectId * OriginalRuntime.ObjectDefinitionBytes;

            if (definitionOffset < 0 ||
                definitionOffset >
                    imgData.Length - OriginalRuntime.ObjectDefinitionBytes)
            {
                return false;
            }

            definition =
                OriginalObjectDefinitionCatalog.Parse(imgData, definitionOffset);
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
                    out var definition))
            {
                return false;
            }

            return catalog.TryGetOrAdd(
                sourceKey,
                definition,
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
    }
}
