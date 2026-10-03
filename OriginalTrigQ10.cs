using System;
using System.IO;
using System.Security.Cryptography;

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

        public const string ExpectedNite3w110Sha256 =
            "12fe5168783446275802e0e947898261b5eca6b88288f3a895fc1faa4c544481";
        public const int Nite3w110DataSegmentFileOffset = 0x2C040;

        static string Sha256Hex(byte[] data)
        {
            using (SHA256 sha = SHA256.Create())
            {
                byte[] digest = sha.ComputeHash(data);
                return BitConverter.ToString(digest)
                    .Replace("-", "")
                    .ToLowerInvariant();
            }
        }

        static short ReadExeInt16(
            byte[] exe,
            int dataBase,
            int offset)
        {
            int p = dataBase + offset;
            if (p < 0 || p > exe.Length - 2)
                throw new InvalidDataException(
                    "NITE3W trig read lies outside executable.");

            return unchecked(
                (short)(
                    exe[p] |
                    (exe[p + 1] << 8)));
        }

        static short ExtractSinQ10(
            byte[] exe,
            int dataBase,
            int degrees)
        {
            int a = Normalize(degrees);

            if (a < 90)
                return ReadExeInt16(exe, dataBase, 0x25E + 2 * a);
            if (a < 180)
                return ReadExeInt16(exe, dataBase, 0x3C6 - 2 * a);
            if (a < 270)
                return unchecked((short)-ReadExeInt16(
                    exe, dataBase, 0x0F6 + 2 * a));

            return unchecked((short)-ReadExeInt16(
                exe, dataBase, 0x52E - 2 * a));
        }

        static short ExtractCosQ10(
            byte[] exe,
            int dataBase,
            int degrees)
        {
            int a = Normalize(degrees);

            if (a < 90)
                return ReadExeInt16(exe, dataBase, 0x314 + 2 * a);
            if (a < 180)
                return unchecked((short)-ReadExeInt16(
                    exe, dataBase, 0x47C - 2 * a));
            if (a < 270)
                return unchecked((short)-ReadExeInt16(
                    exe, dataBase, 0x1AC + 2 * a));

            return ReadExeInt16(exe, dataBase, 0x5E4 - 2 * a);
        }

        public static OriginalTrigQ10 LoadFromNite3w110Exe(
            string path)
        {
            byte[] exe = File.ReadAllBytes(path);

            string digest = Sha256Hex(exe);
            if (digest != ExpectedNite3w110Sha256)
            {
                throw new InvalidDataException(
                    "NITE3W.EXE hash does not match audited Win16 1.10.");
            }

            OriginalTrigQ10 result =
                new OriginalTrigQ10();

            result.SourcePath =
                Path.GetFullPath(path);

            for (int i = 0; i < AngleCount; i++)
            {
                result.sin[i] =
                    ExtractSinQ10(
                        exe,
                        Nite3w110DataSegmentFileOffset,
                        i);

                result.cos[i] =
                    ExtractCosQ10(
                        exe,
                        Nite3w110DataSegmentFileOffset,
                        i);
            }

            if (result.Sin(0) != 0 ||
                result.Sin(45) != 724 ||
                result.Sin(90) != 1024 ||
                result.Cos(0) != 1024 ||
                result.Cos(90) != 0)
            {
                throw new InvalidDataException(
                    "NITE3W.EXE Q10 trig extraction failed sanity checks.");
            }

            return result;
        }

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