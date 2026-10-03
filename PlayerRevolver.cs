namespace Nitemare3D
{
    public class PlayerRevolver : PlayerWeapon
    {
        public PlayerRevolver()
        {
            texture = ImageConsts.UI_REVOLVER;
            fireSound = SoundConsts.WEAPON_REVOLVER01;
            fireTime = .5f;
        }

        public override bool Fire()
        {
            OriginalRuntimeState.FireHitscan(
                OriginalWeaponSelector.SilverPistol);
            return true;
        }
    }
}