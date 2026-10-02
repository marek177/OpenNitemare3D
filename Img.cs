using System;
using System.IO;
using System.Collections.Generic;
using SFML.Graphics;

namespace Nitemare3D
{
    public class BitmapImage
    {
        public byte width, height;
        public byte[,] data;
    }

    public class Img
    {
        const int DirectoryEntries = 256;
        const long WallDirectoryOffset = 0x000;
        const long ObjectDirectoryOffset = 0x400;
        const long ImageIndexRegionEnd = 0x800;

        public static Img current;
        public List<BitmapImage> entries = new List<BitmapImage>();

        readonly UInt32[] wallSlotOffsets = new UInt32[DirectoryEntries];
        readonly UInt32[] objectSlotOffsets = new UInt32[DirectoryEntries];
        readonly int[] wallFrameIndices = new int[DirectoryEntries];
        readonly int[] objectFrameIndices = new int[DirectoryEntries];

        readonly Dictionary<UInt32, int> exactOffsetToFrame =
            new Dictionary<UInt32, int>();

        public int GetWallFrameIndex(int id)
        {
            if (id < 0 || id >= DirectoryEntries)
                return -1;
            return wallFrameIndices[id];
        }

        public int GetObjectFrameIndex(int id)
        {
            if (id < 0 || id >= DirectoryEntries)
                return -1;
            return objectFrameIndices[id];
        }

        public UInt32 GetWallFrameOffset(int id)
        {
            if (id < 0 || id >= DirectoryEntries)
                return 0;
            return wallSlotOffsets[id];
        }

        void ReadDirectory(BinaryReader reader, long baseOffset, UInt32[] output)
        {
            reader.BaseStream.Position = baseOffset;
            for (int i = 0; i < DirectoryEntries; i++)
                output[i] = reader.ReadUInt32();
        }

        void LoadEntries(BinaryReader reader, UInt32 firstDataOffset)
        {
            reader.BaseStream.Position = firstDataOffset;

            while (reader.BaseStream.Position < reader.BaseStream.Length)
            {
                long frameOffset = reader.BaseStream.Position;
                if (frameOffset + 10 > reader.BaseStream.Length)
                    throw new InvalidDataException("Truncated IMG frame header.");

                BitmapImage image = new BitmapImage();

                image.width = reader.ReadByte();
                image.height = reader.ReadByte();

                // Eight original per-frame metadata bytes are not needed by the
                // current renderer, but they are part of the 10-byte header.
                reader.BaseStream.Position += 8;

                long pixelCount = (long)image.width * image.height;
                if (reader.BaseStream.Position + pixelCount > reader.BaseStream.Length)
                    throw new InvalidDataException("IMG frame extends past EOF.");

                image.data = new byte[image.width, image.height];

                for (int x = 0; x < image.width; x++)
                {
                    for (int y = 0; y < image.height; y++)
                    {
                        image.data[x, y] = reader.ReadByte();
                    }
                }

                int frameIndex = entries.Count;
                entries.Add(image);
                exactOffsetToFrame[(UInt32)frameOffset] = frameIndex;
            }
        }

        void ResolveDirectories()
        {
            for (int i = 0; i < DirectoryEntries; i++)
            {
                wallFrameIndices[i] = -1;
                objectFrameIndices[i] = -1;

                if (wallSlotOffsets[i] != 0 &&
                    exactOffsetToFrame.TryGetValue(wallSlotOffsets[i], out int wallIndex))
                {
                    wallFrameIndices[i] = wallIndex;
                }

                if (objectSlotOffsets[i] != 0 &&
                    exactOffsetToFrame.TryGetValue(objectSlotOffsets[i], out int objectIndex))
                {
                    objectFrameIndices[i] = objectIndex;
                }
            }
        }

        public Img(string file)
        {
            using (BinaryReader reader = new BinaryReader(File.OpenRead(file)))
            {
                if (reader.BaseStream.Length < ImageIndexRegionEnd)
                    throw new InvalidDataException("IMG file is too small for both image directories.");

                ReadDirectory(reader, WallDirectoryOffset, wallSlotOffsets);
                ReadDirectory(reader, ObjectDirectoryOffset, objectSlotOffsets);

                UInt32 firstDataOffset = wallSlotOffsets[1];
                if (firstDataOffset < ImageIndexRegionEnd ||
                    firstDataOffset >= reader.BaseStream.Length)
                {
                    throw new InvalidDataException("IMG first image offset is invalid.");
                }

                LoadEntries(reader, firstDataOffset);
                ResolveDirectories();
            }

            current = this;
        }
    }
}
