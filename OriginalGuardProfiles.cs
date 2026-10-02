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
        public static bool TryClassFromMapObjectId(byte mapObjectId, out byte objectClass)
        {
            if (mapObjectId >= 128 && mapObjectId <= 139)
            {
                objectClass = (byte)(0x08 + ((mapObjectId - 128) / 4));
                return true;
            }

            if (mapObjectId >= 144 && mapObjectId <= 159)
            {
                objectClass = (byte)(0x0B + ((mapObjectId - 144) / 4));
                return true;
            }

            if (mapObjectId >= 160 && mapObjectId <= 167)
            {
                objectClass = 0x0F;
                return true;
            }

            if (mapObjectId >= 168 && mapObjectId <= 175)
            {
                objectClass = 0x10;
                return true;
            }

            if (mapObjectId >= 176 && mapObjectId <= 187)
            {
                objectClass = (byte)(0x11 + ((mapObjectId - 176) / 4));
                return true;
            }

            if (mapObjectId >= 188 && mapObjectId <= 195)
            {
                objectClass = 0x15;
                return true;
            }

            if (mapObjectId >= 196 && mapObjectId <= 203)
            {
                objectClass = 0x16;
                return true;
            }

            if (mapObjectId >= 204 && mapObjectId <= 207)
            {
                objectClass = 0x19;
                return true;
            }

            // MAP object 140 is Dancers / scripted GUARD26 and is deliberately
            // not folded into the ordinary GUARD1..25 class mapping here.
            objectClass = 0;
            return false;
        }

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
