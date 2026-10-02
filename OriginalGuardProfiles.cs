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
    /// guard creation path. PerceptionMode is the confirmed GUARD +0x16 selector:
    /// 0 uses one-tile proximity while 1/2 use the perception/LOS result.
    /// </summary>
    public static class OriginalGuardProfiles
    {
        public static bool TryClassFromMapObjectId(
            byte mapObjectId,
            out byte objectClass)
        {
            // Compatibility overload for callers that explicitly mean episode 1.
            return TryClassFromMapObjectId(1, mapObjectId, out objectClass);
        }

        public static bool TryClassFromMapObjectId(
            int episode,
            byte mapObjectId,
            out byte objectClass)
        {
            objectClass = 0;

            if (episode == 1 || episode == 2)
            {
                // Shared GUARD1..3 block.
                if (mapObjectId >= 0x80 && mapObjectId <= 0x8B)
                {
                    objectClass = (byte)(0x08 + ((mapObjectId - 0x80) / 4));
                    return true;
                }

                // 0x8C is the episode-1 Dancers/GUARD26 script and is deliberately
                // outside the ordinary GUARD1..25 class mapping.
                if (mapObjectId >= 0x90 && mapObjectId <= 0x9F)
                {
                    objectClass = (byte)(0x0B + ((mapObjectId - 0x90) / 4));
                    return true;
                }

                if (mapObjectId >= 0xA0 && mapObjectId <= 0xA7)
                {
                    objectClass = 0x0F;
                    return true;
                }

                if (mapObjectId >= 0xA8 && mapObjectId <= 0xAF)
                {
                    objectClass = 0x10;
                    return true;
                }

                if (mapObjectId >= 0xB0 && mapObjectId <= 0xBB)
                {
                    objectClass = (byte)(0x11 + ((mapObjectId - 0xB0) / 4));
                    return true;
                }

                if (episode == 1)
                {
                    if (mapObjectId >= 0xBC && mapObjectId <= 0xC3)
                    {
                        objectClass = 0x15; // GUARD14 Penelope
                        return true;
                    }

                    if (mapObjectId >= 0xC4 && mapObjectId <= 0xCB)
                    {
                        objectClass = 0x16; // GUARD15 Dr. Hamerstein
                        return true;
                    }

                    if (mapObjectId >= 0xCC && mapObjectId <= 0xCF)
                    {
                        objectClass = 0x19; // GUARD18 Cannon
                        return true;
                    }
                }
                else
                {
                    if (mapObjectId >= 0xBC && mapObjectId <= 0xBF)
                    {
                        objectClass = 0x17; // GUARD16 Tall slim robot
                        return true;
                    }

                    if (mapObjectId >= 0xC0 && mapObjectId <= 0xC7)
                    {
                        objectClass = 0x18; // GUARD17 Trashcan robot
                        return true;
                    }

                    if (mapObjectId >= 0xC8 && mapObjectId <= 0xCB)
                    {
                        objectClass = 0x19; // GUARD18 Cannon
                        return true;
                    }
                }

                return false;
            }

            if (episode == 3)
            {
                if (mapObjectId >= 0x70 && mapObjectId <= 0x77)
                {
                    objectClass = 0x15; // GUARD14 Penelope
                    return true;
                }

                if (mapObjectId >= 0x78 && mapObjectId <= 0x7F)
                {
                    objectClass = 0x16; // GUARD15 Dr. Hamerstein
                    return true;
                }

                if (mapObjectId >= 0x80 && mapObjectId <= 0x83)
                {
                    objectClass = 0x1A; // GUARD19 Ghost
                    return true;
                }

                if (mapObjectId >= 0x84 && mapObjectId <= 0x87)
                {
                    objectClass = 0x1B; // GUARD20 Goldie
                    return true;
                }

                if (mapObjectId >= 0x88 && mapObjectId <= 0x8B)
                {
                    objectClass = 0x1C; // GUARD21 Greenie
                    return true;
                }

                if (mapObjectId >= 0x8C && mapObjectId <= 0x8F)
                {
                    objectClass = 0x1D; // GUARD22 Demon
                    return true;
                }

                if (mapObjectId >= 0x90 && mapObjectId <= 0x97)
                {
                    objectClass = 0x1E; // GUARD23 Alien #1
                    return true;
                }

                if (mapObjectId >= 0x98 && mapObjectId <= 0x9F)
                {
                    objectClass = 0x1F; // GUARD24 Alien #2
                    return true;
                }
            }

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
