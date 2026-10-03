using System;

namespace Nitemare3D
{
    /// <summary>
    /// Win16 NITE3W shade/remap runtime from FUN_1010_29BE.
    /// DAT_7E60 selects one of eight shade levels; DAT_8094 is the active
    /// 256-byte pixel remap. The audited WinG path remaps palette indices
    /// 10..245 against the same 236-color base palette.
    /// </summary>
    public sealed class OriginalShadeRuntime
    {
        static readonly int[] Levels =
            { 0, 4, 8, 12, 16, 20, 30, 40 };

        public readonly byte[] Remap =
            new byte[256];

        public ushort ShadeIndex { get; private set; } = 2;

        public OriginalShadeRuntime()
        {
            ResetIdentity();
        }

        public static int ShadeLevel(int shadeIndex)
        {
            if (shadeIndex < 0 ||
                shadeIndex >= Levels.Length)
            {
                throw new ArgumentOutOfRangeException(
                    nameof(shadeIndex));
            }

            return Levels[shadeIndex];
        }

        public void Rebuild(
            byte[] palette,
            int shadeIndex)
        {
            if (palette == null ||
                palette.Length < 256 * 3)
            {
                throw new ArgumentException(
                    "A 256-entry RGB palette is required.",
                    nameof(palette));
            }

            if (shadeIndex < 0 ||
                shadeIndex >= Levels.Length)
            {
                throw new ArgumentOutOfRangeException(
                    nameof(shadeIndex));
            }

            ShadeIndex =
                (ushort)shadeIndex;

            ResetIdentity();

            if (shadeIndex == 0)
                return;

            int subtract =
                Levels[shadeIndex] * 4;

            for (int source = 10;
                source <= 245;
                source++)
            {
                int sourceOffset =
                    source * 3;

                int r =
                    Math.Max(
                        0,
                        palette[sourceOffset + 0] -
                        subtract);
                int g =
                    Math.Max(
                        0,
                        palette[sourceOffset + 1] -
                        subtract);
                int b =
                    Math.Max(
                        0,
                        palette[sourceOffset + 2] -
                        subtract);

                int bestIndex = 10;
                int bestDistance = int.MaxValue;

                for (int candidate = 10;
                    candidate <= 245;
                    candidate++)
                {
                    int candidateOffset =
                        candidate * 3;

                    int distance =
                        Math.Abs(
                            r -
                            palette[candidateOffset + 0]) +
                        Math.Abs(
                            g -
                            palette[candidateOffset + 1]) +
                        Math.Abs(
                            b -
                            palette[candidateOffset + 2]);

                    // Original scan retains the first strict minimum.
                    if (distance < bestDistance)
                    {
                        bestDistance =
                            distance;
                        bestIndex =
                            candidate;
                    }
                }

                Remap[source] =
                    (byte)bestIndex;
            }
        }

        void ResetIdentity()
        {
            for (int i = 0;
                i < Remap.Length;
                i++)
            {
                Remap[i] =
                    (byte)i;
            }
        }
    }
}
