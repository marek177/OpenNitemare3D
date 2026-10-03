namespace Nitemare3D
{
    public enum PickupType
    {
        RedKey,
        GreenKey,
        BlueKey,
        YellowKey,
        RedIDCard,
        YellowIDCard,
        RedPotion,
        BluePotion,
        Eyeball,
        CrystallBall,
        PlasmaPistol,
        MagicWand,
        Pistol,
        AutoPlasmaPistol,
        SilverBullets,
        PlasmaPowerCell,
        SpellbookWandPower,
        PentagramRed,
        PentagramGreen,
        PentagramBlue,
        PentagramYellow
    }

    public class Pickup : Entity, ISprite
    {
        public int spriteIndex { get; set; }
        public bool visible { get; set; } = true;
        public Vec2 spritePosition { get; set; } = new Vec2();
        public float yOffset { get; set; }

        readonly byte rawObjectId;
        AnimationHandler anim = new AnimationHandler();

        // Frame indices verified against the original IMG.1 object directory
        // at file offset 0x400.
        static Animation[] animations = new Animation[]
        {
            new Animation(189, 1, 125), // red key
            new Animation(190, 1, 125), // green key
            new Animation(191, 1, 125), // blue key
            new Animation(192, 1, 125), // yellow key
            new Animation(193, 1, 125), // red card
            new Animation(194, 1, 125), // yellow card
            new Animation(204, 4, 125), // red potion
            new Animation(208, 4, 125), // blue potion
            new Animation(215, 6, 125), // magic eye
            new Animation(221, 6, 125), // crystal ball
            new Animation(235, 1, 125), // single-shot plasma
            new Animation(236, 1, 125), // magic wand
            new Animation(237, 1, 125), // silver pistol
            new Animation(238, 1, 125), // multi-bolt plasma
            new Animation(239, 1, 125), // silver bullets
            new Animation(240, 1, 125), // plasma power cell
            new Animation(241, 1, 125), // spellbook / wand power
            new Animation(231, 1, 125), // red pentagram
            new Animation(232, 1, 125), // green pentagram
            new Animation(233, 1, 125), // blue pentagram
            new Animation(234, 1, 125)  // yellow pentagram
        };

        PickupType type;

        public Pickup(PickupType type, byte rawObjectId = 0)
        {
            this.type = type;
            this.rawObjectId = rawObjectId;

            anim.LoadAnimation(animations[(int)type]);
            Game.player.AddSprite(this);
            anim.index = animations[(int)type].frames[0].index;
            hasCollision = false;

            if (type == PickupType.Eyeball)
                yOffset = 32;
        }

        public override void Update()
        {
            anim.Update();
            spriteIndex = anim.index;
            spritePosition = position;

            if ((int)position.X == (int)Game.player.position.X &&
                (int)position.Y == (int)Game.player.position.Y)
            {
                OnTouchPlayer();
            }
        }

        bool TryOriginalCf60Pickup()
        {
            if (rawObjectId == 0 || Level.originalMap == null)
                return false;

            if (!Level.originalMap.TryGetObjectClassAndVariant(
                    rawObjectId,
                    out byte objectClass,
                    out byte variant))
            {
                return false;
            }

            // A MAP-backed pickup in one of the recovered CF60 direct classes
            // must not fall through to legacy behavior when its effect is rejected.
            bool recoveredClass =
                objectClass == 0x2F ||
                objectClass == 0x30 ||
                objectClass == 0x33 ||
                objectClass == 0x36 ||
                objectClass == 0x39 ||
                objectClass == 0x3A ||
                objectClass == 0x3B ||
                objectClass == 0x3C;

            if (!recoveredClass)
                return false;

            bool consumed =
                OriginalRuntimeState.PickupRuntime.TryCollectDirectMapPickup(
                    objectClass,
                    variant,
                    Game.player,
                    OriginalRuntimeState.WeaponRuntime);

            if (!consumed)
                return true; // handled by CF60, but pickup remains in the world.

            int soundEvent =
                OriginalPickupRuntime.PickupSoundEventId(objectClass);
            if (soundEvent >= 0)
                SoundEffect.PlayOriginalEvent(soundEvent);

            Entity.Remove(this);
            visible = false;
            return true;
        }

        void OnTouchPlayer()
        {
            if (TryOriginalCf60Pickup())
                return;

            // Compatibility path for manually-created legacy Pickup instances.
            switch (type)
            {
                case PickupType.RedKey:
                case PickupType.GreenKey:
                case PickupType.BlueKey:
                case PickupType.YellowKey:
                case PickupType.RedIDCard:
                case PickupType.YellowIDCard:
                    SoundEffect.PlaySound(SoundConsts.PICKUP_KEY);
                    break;

                case PickupType.Eyeball:
                    SoundEffect.PlaySound(SoundConsts.PICKUP_EYE);
                    break;

                case PickupType.CrystallBall:
                    SoundEffect.PlaySound(SoundConsts.PICKUP_GLASSBALL);
                    break;

                case PickupType.PlasmaPistol:
                    SoundEffect.PlaySound(SoundConsts.PICKUP_WEAPON);
                    Game.player.AcquireWeapon(
                        OriginalWeaponSelector.SingleShotLaser);
                    break;

                case PickupType.MagicWand:
                    SoundEffect.PlaySound(SoundConsts.PICKUP_WEAPON);
                    Game.player.AcquireWeapon(
                        OriginalWeaponSelector.MagicWand);
                    break;

                case PickupType.Pistol:
                    SoundEffect.PlaySound(SoundConsts.PICKUP_WEAPON);
                    Game.player.AcquireWeapon(
                        OriginalWeaponSelector.SilverPistol);
                    break;

                case PickupType.AutoPlasmaPistol:
                    SoundEffect.PlaySound(SoundConsts.PICKUP_WEAPON);
                    Game.player.AcquireWeapon(
                        OriginalWeaponSelector.ContinuousLaser);
                    break;
            }

            Entity.Remove(this);
            visible = false;
        }
    }
}
