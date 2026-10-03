namespace Nitemare3D
{
    public class PlayerAutoPistol : PlayerWeapon
    {
        public PlayerAutoPistol()
        {
            texture = ImageConsts.UI_AUTOPLASMAGUN;
            fireTime = .2f;
            fireSound = SoundConsts.WEAPON_PLASMA;
        }

        public override void Fire()
        {
            var p = new Projectile(
                Game.player.direction,
                ProjectileType.Plasma,
                OriginalWeaponSelector.ContinuousLaser);
            Entity.Add(p, Game.player.position);
        }
    }
}