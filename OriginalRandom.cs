namespace Nitemare3D
{
    // Original Win16/DOS-compatible Microsoft C rand() stream used by Nitemare-3D.
    public static class OriginalRandom
    {
        static uint state = 1;

        public static uint State => state;

        public static void Reset(uint seed = 1)
        {
            state = seed;
        }

        public static int Next()
        {
            unchecked
            {
                state = state * 0x343FDu + 0x269EC3u;
            }

            return (int)((state >> 16) & 0x7FFFu);
        }
    }
}
