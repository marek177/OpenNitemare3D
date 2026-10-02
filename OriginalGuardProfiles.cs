namespace Nitemare3D
{
    public struct OriginalGuardInitialProfile
    {
        public byte Strategy;
        public byte State;
        public byte NextState;
        public byte PerceptionMode;
    }

    /// <summary>
    /// Class-specific initialization assignments recovered from the Win16
    /// guard creation path. PerceptionMode is kept as a neutral recovered value
    /// until its destination field semantics are fully named.
    /// </summary>
    public static class OriginalGuardProfiles
    {
        public static OriginalGuardInitialProfile ForObjectClass(byte objectClass)
        {
            var profile = new OriginalGuardInitialProfile
            {
                Strategy = 0,
                State = (byte)OriginalGuardState.Active07,
                NextState = (byte)OriginalGuardState.Active02,
                PerceptionMode = 1
            };

            switch (objectClass)
            {
                case 0x08:
                case 0x09:
                case 0x0A:
                case 0x11:
                case 0x14:
                case 0x1A:
                    profile.PerceptionMode = 0;
                    break;

                case 0x12:
                case 0x13:
                    profile.Strategy = 3;
                    profile.PerceptionMode = 0;
                    break;

                case 0x15:
                case 0x16:
                    profile.NextState = 0;
                    break;

                case 0x19:
                    profile.Strategy = 4;
                    profile.State = (byte)OriginalGuardState.Conditional0E;
                    break;

                case 0x21:
                    profile.State = 0;
                    profile.NextState = 0;
                    break;
            }

            return profile;
        }
    }
}
