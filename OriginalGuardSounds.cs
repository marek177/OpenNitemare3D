namespace Nitemare3D
{
    /// <summary>
    /// Exact class-specific SND selectors recovered from NITE3W.EXE V1.10:
    /// B862 = alert, B5E4 = attack, B6A0 = death.
    /// The returned values are original SND.DAT indices.
    /// </summary>
    public static class OriginalGuardSounds
    {
        public static int AlertSoundId(byte objectClass, ushort randomValue)
        {
            switch (objectClass)
            {
                case 0x08: return 0x22;
                case 0x09:
                case 0x0A: return randomValue % 3 + 0x38;
                case 0x0B: return 0x3B;
                case 0x0C: return 0x14;
                case 0x0D: return 0x15;
                case 0x0E: return 0x10;
                case 0x0F:
                case 0x10: return randomValue % 2 + 0x36;
                case 0x11: return 0x06;
                case 0x12: return 0x3F;
                case 0x13: return 0x3C;
                case 0x16: return 0x12;
                case 0x17: return 0x48;
                case 0x18: return 0x47;
                case 0x1A: return 0x49;
                case 0x1B:
                case 0x1C: return 0x0D;
                case 0x1D: return 0x38;
                case 0x1E:
                case 0x1F: return 0x3D;
                default: return 0;
            }
        }

        public static int AttackSoundId(byte objectClass, ushort randomValue)
        {
            switch (objectClass)
            {
                case 0x09:
                case 0x0A: return 0x41;
                case 0x0B:
                case 0x1A: return 0x4E;
                case 0x0C:
                case 0x0D:
                case 0x0E:
                case 0x18: return 0x20;
                case 0x0F:
                case 0x10:
                case 0x16: return randomValue % 3 + 0x17;
                case 0x12: return 0x3E;
                case 0x13: return 0x40;
                case 0x17: return 0x1F;
                case 0x19: return 0x1D;
                case 0x1B:
                case 0x1C:
                case 0x1D:
                case 0x1E:
                case 0x1F: return randomValue % 4 + 0x4B;
                default: return 0;
            }
        }

        public static int DeathSoundId(byte objectClass, ushort randomValue)
        {
            switch (objectClass)
            {
                case 0x08:
                case 0x14: return 0x23;
                case 0x09: return 0x08;
                case 0x0A:
                case 0x12:
                case 0x13: return 0x07;
                case 0x0B: return 0x24;
                case 0x0C: return 0x04;
                case 0x0D: return 0x13;
                case 0x0E: return 0x0E;
                case 0x0F:
                case 0x10: return randomValue % 3 + 0x0B;
                case 0x17:
                case 0x18:
                case 0x1E:
                case 0x1F: return 0x46;
                case 0x1A: return 0x4A;
                case 0x1B:
                case 0x1C: return 0x02;
                case 0x1D: return 0x09;
                default: return 0;
            }
        }
    }
}
