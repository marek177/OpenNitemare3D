using System;
using System.IO;

namespace Nitemare3D
{
    /// <summary>
    /// Exact 360-degree Q10 sine/cosine table extracted from the hashed
    /// NITE3W Win16 1.10 data segment.
    ///
    /// File format N3D_TRIG_Q10.BIN:
    ///   360 little-endian signed Int16 sine values
    ///   360 little-endian signed Int16 cosine values
    /// </summary>
    public sealed class OriginalTrigQ10
    {
        public const int AngleCount = 360;
        public const int FileSize = 1440;

        readonly short[] sin =
            new short[AngleCount];

        readonly short[] cos =
            new short[AngleCount];

        public string SourcePath { get; private set; }

        public static OriginalTrigQ10 Load(
            string path)
        {
            byte[] bytes =
                File.ReadAllBytes(path);

            if (bytes.Length != FileSize)
            {
                throw new InvalidDataException(
                    "N3D_TRIG_Q10.BIN must be exactly 1440 bytes.");
            }

            OriginalTrigQ10 result =
                new OriginalTrigQ10();

            result.SourcePath =
                Path.GetFullPath(path);

            for (int i = 0;
                i < AngleCount;
                i++)
            {
                result.sin[i] =
                    ReadInt16LE(
                        bytes,
                        i * 2);

                result.cos[i] =
                    ReadInt16LE(
                        bytes,
                        720 + i * 2);
            }

            if (result.Sin(0) != 0 ||
                result.Sin(45) != 724 ||
                result.Sin(90) != 1024 ||
                result.Cos(0) != 1024 ||
                result.Cos(90) != 0)
            {
                throw new InvalidDataException(
                    "Q10 trig table failed NITE3W cardinal/45-degree sanity checks.");
            }

            return result;
        }

        public short Sin(int degrees)
        {
            return sin[Normalize(degrees)];
        }

        public short Cos(int degrees)
        {
            return cos[Normalize(degrees)];
        }

        public static int Normalize(
            int degrees)
        {
            int value = degrees % 360;

            if (value < 0)
            {
                value += 360;
            }

            return value;
        }

        static short ReadInt16LE(
            byte[] bytes,
            int offset)
        {
            return unchecked(
                (short)(
                    bytes[offset] |
                    (bytes[offset + 1] << 8)));
        }
    }
}