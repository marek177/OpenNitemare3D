using System;
using System.IO;

namespace Nitemare3D
{
    public struct OriginalImgResource
    {
        public byte ImageId;
        public bool TileBank;
        public uint FrameStreamOffset;
        public OriginalObjectDefinitionRecord Header;
        public byte[] RuntimeFrameTable;
    }

    /// <summary>
    /// Clean-room reader for the IMG resource path recovered from
    /// FUN_1010_4C8A -> FUN_1010_4B86 -> FUN_1010_4AB0.
    ///
    /// Layout used here:
    /// - file +0x0000: 256 dword tile/VEC frame-stream offsets
    /// - file +0x0400: 256 dword object frame-stream offsets
    /// - file +0x0800: 256 x 0x5A tile/VEC headers
    /// - immediately following: 256 x 0x5A object headers
    /// - each directory offset points at a sequence of raw 10-byte frame records
    ///   followed by width*height bytes of pixel data per frame
    /// </summary>
    public static class OriginalImgResourceReader
    {
        public static bool TryReadResource(
            string path,
            byte imageId,
            bool tileBank,
            out OriginalImgResource resource)
        {
            resource = default;

            using (var stream = File.OpenRead(path))
            using (var reader = new BinaryReader(stream))
            {
                if (stream.Length < OriginalRuntime.ImgDirectoryBytes)
                    return false;

                long directoryOffset =
                    (tileBank
                        ? OriginalRuntime.ImgWallDirectoryOffset
                        : OriginalRuntime.ImgObjectDirectoryOffset) +
                    imageId * 4L;
                stream.Position = directoryOffset;
                uint frameStreamOffset = reader.ReadUInt32();
                if (frameStreamOffset == 0 ||
                    frameStreamOffset >= stream.Length)
                {
                    return false;
                }

                int headerOffset =
                    OriginalObjectDefinitionCatalog.HeaderOffset(imageId, tileBank);
                if (headerOffset < 0 ||
                    headerOffset + OriginalRuntime.ObjectResourceHeaderBytes > stream.Length)
                {
                    return false;
                }

                stream.Position = headerOffset;
                byte[] headerBytes =
                    reader.ReadBytes(OriginalRuntime.ObjectResourceHeaderBytes);
                if (headerBytes.Length != OriginalRuntime.ObjectResourceHeaderBytes)
                    return false;

                var header =
                    OriginalObjectDefinitionCatalog.ParseHeader(headerBytes, 0);

                byte[] runtimeTable =
                    new byte[header.FrameCount * OriginalRuntime.ObjectResourceEntryBytes];

                stream.Position = frameStreamOffset;

                for (int i = 0; i < header.FrameCount; i++)
                {
                    if (stream.Position >
                        stream.Length - OriginalRuntime.ObjectResourceEntryBytes)
                    {
                        return false;
                    }

                    byte[] raw =
                        reader.ReadBytes(OriginalRuntime.ObjectResourceEntryBytes);
                    if (raw.Length != OriginalRuntime.ObjectResourceEntryBytes)
                        return false;

                    byte width = raw[0];
                    byte height = raw[1];

                    if (tileBank)
                    {
                        if (height != 0x40 ||
                            (width != 0x40 && width != 0x80))
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (width * height > 0x0C00)
                            return false;
                    }

                    long pixelDataOffset = stream.Position;
                    long pixelBytes = (long)width * height;
                    if (pixelDataOffset < 0 ||
                        pixelDataOffset > uint.MaxValue ||
                        pixelBytes < 0 ||
                        pixelDataOffset + pixelBytes > stream.Length)
                    {
                        return false;
                    }

                    int entryOffset = i * OriginalRuntime.ObjectResourceEntryBytes;
                    runtimeTable[entryOffset + 0] = width;
                    runtimeTable[entryOffset + 1] = height;
                    WriteUInt32LittleEndian(
                        runtimeTable,
                        entryOffset + 2,
                        (uint)pixelDataOffset);

                    // FUN_4AB0 clears the cached-pixel far pointer at +06.
                    WriteUInt32LittleEndian(runtimeTable, entryOffset + 6, 0);

                    stream.Position = pixelDataOffset + pixelBytes;
                }

                resource = new OriginalImgResource
                {
                    ImageId = imageId,
                    TileBank = tileBank,
                    FrameStreamOffset = frameStreamOffset,
                    Header = header,
                    RuntimeFrameTable = runtimeTable
                };
                return true;
            }
        }

        public static OriginalImageFrameRuntimeRecord DecodeRuntimeFrame(
            byte[] runtimeTable,
            int frameIndex)
        {
            if (runtimeTable == null)
                throw new ArgumentNullException(nameof(runtimeTable));
            if (frameIndex < 0)
                throw new ArgumentOutOfRangeException(nameof(frameIndex));

            int offset = frameIndex * OriginalRuntime.ObjectResourceEntryBytes;
            if (offset > runtimeTable.Length - OriginalRuntime.ObjectResourceEntryBytes)
                throw new ArgumentOutOfRangeException(nameof(frameIndex));

            return new OriginalImageFrameRuntimeRecord
            {
                Width = runtimeTable[offset + 0],
                Height = runtimeTable[offset + 1],
                PixelDataFileOffset = ReadUInt32LittleEndian(runtimeTable, offset + 2),
                CachedPixelsPointer = ReadUInt32LittleEndian(runtimeTable, offset + 6)
            };
        }

        static uint ReadUInt32LittleEndian(byte[] data, int offset)
        {
            return (uint)(data[offset] |
                          (data[offset + 1] << 8) |
                          (data[offset + 2] << 16) |
                          (data[offset + 3] << 24));
        }

        static void WriteUInt32LittleEndian(byte[] data, int offset, uint value)
        {
            data[offset + 0] = (byte)value;
            data[offset + 1] = (byte)(value >> 8);
            data[offset + 2] = (byte)(value >> 16);
            data[offset + 3] = (byte)(value >> 24);
        }
    }
}
