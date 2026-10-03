namespace Nitemare3D
{
    /// <summary>
    /// Microsoft 16-bit C runtime rand()/srand() path used by NITE3W 1.10.
    ///
    /// FUN_1018_32D2 -> FUN_1008_6EC8:
    ///   seed = seed * 0x343FD + 0x269EC3
    ///   return (seed >> 16) & 0x7FFF
    ///
    /// FUN_1018_32D8 explicitly seeds the generator with 1.
    /// </summary>
    public static class OriginalRandom
    {
        public const uint Multiplier = 0x000343FDu;
        public const uint Increment = 0x00269EC3u;
        public const uint OriginalSeed = 1u;

        static uint seed = OriginalSeed;

        public static uint Seed
        {
            get { return seed; }
        }

        public static void ResetToOriginalSeed()
        {
            seed = OriginalSeed;
        }

        public static void SetSeed(ushort value)
        {
            // The original srand wrapper accepts a 16-bit argument and clears
            // the high word of the 32-bit CRT seed.
            seed = value;
        }

        public static ushort Next()
        {
            seed = unchecked(seed * Multiplier + Increment);
            return (ushort)((seed >> 16) & 0x7FFF);
        }
    }
}
