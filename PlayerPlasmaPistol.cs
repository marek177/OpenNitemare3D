namespace Nitemare3D
{
    public class PlayerPlasmaPistol : PlayerWeapon
    {
        public PlayerPlasmaPistol()
        {
            texture = ImageConsts.UI_PLASMAGUN;
            fireTime = .3f;
            fireSound = SoundConsts.WEAPON_PLASMA;
        }
        public override bool Fire()
        {
            return Projectile.TrySpawn(
                Game.player.direction,
                ProjectileType.Plasma,
                OriginalWeaponSelector.SingleShotLaser,
                Game.player.position);
        }
    }
}