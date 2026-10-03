namespace Nitemare3D
{
    public class PlayerMagicWand : PlayerWeapon
    {
        public PlayerMagicWand()
        {
            texture = ImageConsts.UI_MAGICWAND;
            fireTime = .3f;
            fireSound = SoundConsts.WEAPON_MAGICWAND;
        }
        public override bool Fire()
        {
            return Projectile.TrySpawn(
                Game.player.direction,
                ProjectileType.Magic,
                OriginalWeaponSelector.MagicWand,
                Game.player.position);
        }
    }
}