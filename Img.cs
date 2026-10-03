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
        public static Img current;
        public List<BitmapImage> entries = new List<BitmapImage>();

        public byte[] rawData { get; private set; }
        public int episode { get; private set; }

        void LoadEntries(List<UInt32> offsets, BinaryReader reader)
        {
            reader.BaseStream.Position = offsets[0];
            while (reader.BaseStream.Position != reader.BaseStream.Length)
            {
                BitmapImage image = new BitmapImage();

                image.width = reader.ReadByte();
                image.height = reader.ReadByte();

                image.data = new byte[image.width, image.height];

                reader.BaseStream.Position += 8;

                for (int x = 0; x < image.width; x++)
                {
                    for (int y = 0; y < image.height; y++)
                    {
                        image.data[x, y] = reader.ReadByte();
                    }
                }

                entries.Add(image);
            }
        }

        static int InferEpisode(string file)
        {
            string extension = Path.GetExtension(file);
            if (extension.Length == 2 &&
                extension[1] >= '1' &&
                extension[1] <= '3')
            {
                return extension[1] - '0';
            }

            return 0;
        }

        public static Img LoadEpisode(int episode)
        {
            if (episode < 1 || episode > 3)
                throw new ArgumentOutOfRangeException(nameof(episode));

            if (current != null && current.episode == episode)
                return current;

            return new Img("data/IMG." + episode, episode);
        }

        public bool TryGetObjectDefinition(
            byte objectId,
            out uint sourceKey,
            out byte[] headerBytes,
            out byte[] runtimeFrameTable)
        {
            return OriginalImgDefinitionLoader.TryReadObjectDefinition(
                rawData,
                objectId,
                out sourceKey,
                out headerBytes,
                out runtimeFrameTable);
        }

        public Img(string file, int episode = 0)
        {
            this.episode = episode != 0 ? episode : InferEpisode(file);
            rawData = File.ReadAllBytes(file);

            using (var reader = new BinaryReader(new MemoryStream(rawData)))
            {
                reader.BaseStream.Position = 4;

                // Keep the historical sequential frame loader for rendering.
                // The recovered definition/directory parser uses rawData directly.
                List<UInt32> offsets = new List<UInt32>();
                UInt32 offset;
                UInt32 lowestOffset = 0xFFFFFFFF;

                do
                {
                    offset = reader.ReadUInt32();
                    if (offset != 0)
                    {
                        offsets.Add(offset);
                        if (offset < lowestOffset)
                            lowestOffset = offset;
                    }
                } while (reader.BaseStream.Position <= lowestOffset);

                if (offsets.Count == 0)
                    throw new InvalidDataException("IMG contains no frame offsets.");

                LoadEntries(offsets, reader);
            }

            current = this;
        }
    }
}
