namespace Nitemare3D
{
    /// <summary>
    /// Semantic wall classes stored in MAP.N header +0x0002[256].
    /// Names are cross-checked against WALLS.1/2/3.
    /// </summary>
    public enum OriginalWallSemanticClass : byte
    {
        Null = 0x00,
        Wall = 0x01,
        ReverseWall = 0x02,
        Control = 0x03,

        OneShot = 0x07,
        Special1 = 0x08,
        LevelUp = 0x09,
        LevelUp2 = 0x0A,

        Warp1 = 0x0D,
        Warp2 = 0x0E,
        Warp3 = 0x0F,
        Warp4 = 0x10,
        Warp5 = 0x11,
        Warp6 = 0x12,
        Warp7 = 0x13,
        Warp8 = 0x14,

        WarpSpecial1 = 0x15,
        WarpSpecial2 = 0x16,

        WarpLocked1 = 0x19,
        WarpLocked2 = 0x1A,
        WarpLocked3 = 0x1B,
        WarpLocked4 = 0x1C,

        WarpElevator1 = 0x1D,
        WarpElevator2 = 0x1E,

        ExplodingWall = 0x2D,
        ExplodingWall1 = 0x2E,
        ExplodingWall2 = 0x2F,

        Jamb = 0x30,
        DoorVertical = 0x31,
        DoorHorizontal = 0x32,
        DoorVerticalLocked = 0x33,
        DoorHorizontalLocked = 0x34,
        DoorVerticalLocked2 = 0x35,
        DoorHorizontalLocked2 = 0x36,
        DoorVerticalLocked3 = 0x37,
        DoorHorizontalLocked3 = 0x38,
        DoorVerticalInside = 0x39,
        DoorHorizontalInside = 0x3A,
        DoorVerticalRemote = 0x3B,
        DoorHorizontalRemote = 0x3C,
        DoorVerticalCurtain = 0x3F,
        DoorHorizontalCurtain = 0x40,

        Turn = 0x41,
        Retreat = 0x42,
        Flee = 0x43,
        Floor = 0x44,
        SafeSpot = 0x45,
        ActionSpot = 0x46,
        Trigger1 = 0x47,
        Trigger2 = 0x48
    }

    /// <summary>
    /// Semantic object classes stored in MAP.N header +0x0102[256].
    /// Names are cross-checked against OBJECTS.1/2/3. Class 0x14 is runtime-only:
    /// the original Dracula death finalizer transforms class 0x11 to 0x14.
    /// </summary>
    public enum OriginalObjectSemanticClass : byte
    {
        Null = 0x00,
        Start = 0x02,
        Secret = 0x03,
        Impact = 0x04,
        Missile = 0x05,
        Caustic = 0x07,

        Bat = 0x08,
        Frankenstein = 0x09,
        Mummy = 0x0A,
        Skeleton = 0x0B,
        MrsH = 0x0C,
        Zelda = 0x0D,
        Vampira = 0x0E,
        HumanBlue = 0x0F,
        HumanGreen = 0x10,
        Dracula = 0x11,
        CemeteryGargoyle = 0x12,
        GardenGargoyle = 0x13,
        DraculaBatPhase2 = 0x14,
        Penelope = 0x15,
        DrHamerstein = 0x16,
        TallSlimRobot = 0x17,
        TrashcanRobot = 0x18,
        Cannon = 0x19,
        Ghost = 0x1A,
        Goldie = 0x1B,
        Greenie = 0x1C,
        Demon = 0x1D,
        Alien1 = 0x1E,
        Alien2 = 0x1F,
        Dancers = 0x21,

        Safe = 0x26,
        Trunk = 0x27,
        Push = 0x28,
        Action = 0x29,
        Permeable = 0x2A,
        Dumb = 0x2B,
        Elevated = 0x2E,
        Key = 0x2F,
        IdCard = 0x30,
        Food = 0x33,
        Weapon = 0x36,
        Ammo = 0x39,
        CrystalBall = 0x3A,
        MagicEye = 0x3B,
        Pentagram = 0x3C,
        Scroll = 0x3D,
        UifObject = 0x3E
    }

    public static class OriginalMapSemanticClasses
    {
        public static OriginalWallSemanticClass Wall(byte value)
        {
            return (OriginalWallSemanticClass)value;
        }

        public static OriginalObjectSemanticClass Object(byte value)
        {
            return (OriginalObjectSemanticClass)value;
        }

        public static bool IsGuardClass(byte value)
        {
            return (value >= (byte)OriginalObjectSemanticClass.Bat &&
                    value <= (byte)OriginalObjectSemanticClass.GardenGargoyle) ||
                   (value >= (byte)OriginalObjectSemanticClass.Penelope &&
                    value <= (byte)OriginalObjectSemanticClass.Alien2) ||
                   value == (byte)OriginalObjectSemanticClass.Dancers;
        }

        public static bool IsDirectionalDoorClass(byte value)
        {
            return value >= (byte)OriginalWallSemanticClass.DoorVertical &&
                   value <= (byte)OriginalWallSemanticClass.DoorHorizontalRemote;
        }
    }
}
