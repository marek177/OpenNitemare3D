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

        public override bool Fire()
        {
            return Projectile.TrySpawn(
                Game.player.direction,
                ProjectileType.Plasma,
                OriginalWeaponSelector.ContinuousLaser,
                Game.player.position);
        }
    }
}