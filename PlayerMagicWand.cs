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
        public override void Fire()
        {
            var p = new Projectile(
                Game.player.direction,
                ProjectileType.Magic,
                OriginalWeaponSelector.MagicWand);
            Entity.Add(p, Game.player.position);
        }
    }
}