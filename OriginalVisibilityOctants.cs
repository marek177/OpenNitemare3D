using System;
using System.IO;

namespace Nitemare3D
{
    /// <summary>
    /// The four 8-entry orientation enable tables read by Win16
    /// FUN_1018_3940 at:
    ///
    ///   DS:04C6 + octant  orientation 0
    ///   DS:04CE + octant  orientation 1
    ///   DS:04D6 + octant  orientation 2
    ///   DS:04DE + octant  orientation 3
    ///
    /// File format N3D_VISIBILITY_OCTANTS.BIN:
    ///   4 orientations * 8 octants = 32 bytes, orientation-major.
    /// Any nonzero byte is treated exactly as the original boolean test.
    /// </summary>
    public sealed class OriginalVisibilityOctants
    {
        public const int OrientationCount = 4;
        public const int OctantCount = 8;
        public const int FileSize = 32;

        readonly byte[,] enabled =
            new byte[OrientationCount, OctantCount];

        public string SourcePath { get; private set; }

        public static OriginalVisibilityOctants Load(
            string path)
        {
            byte[] bytes =
                File.ReadAllBytes(path);

            if (bytes.Length != FileSize)
            {
                throw new InvalidDataException(
                    "N3D_VISIBILITY_OCTANTS.BIN must be exactly 32 bytes.");
            }

            OriginalVisibilityOctants result =
                new OriginalVisibilityOctants();

            result.SourcePath =
                Path.GetFullPath(path);

            int p = 0;

            for (int orientation = 0;
                orientation < OrientationCount;
                orientation++)
            {
                for (int octant = 0;
                    octant < OctantCount;
                    octant++)
                {
                    result.enabled[
                        orientation,
                        octant] =
                        bytes[p++];
                }
            }

            return result;
        }

        public bool IsEnabled(
            int orientation,
            int octant)
        {
            if (orientation < 0 ||
                orientation >= OrientationCount)
            {
                throw new ArgumentOutOfRangeException(
                    nameof(orientation));
            }

            octant &= 7;

            return enabled[
                orientation,
                octant] != 0;
        }

        public byte Raw(
            int orientation,
            int octant)
        {
            return enabled[
                orientation,
                octant & 7];
        }
    }
}