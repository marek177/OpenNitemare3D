#include "n3d_re_weapon.h"

#include "n3d_re_projectile.h"

n3d_fire_result N3D_RE_TryBeginPlayerFire(uint8_t projectile_sequence_base)
{
    n3d_fire_result result = {0};
    result.projectile_slot = -1;
    result.weapon_selector = n3d_player.active_weapon;

    if(n3d_player.active_weapon == N3D_WEAPON_NONE ||
       n3d_player.active_weapon >= N3D_WEAPON_COUNT)
    {
        result.kind = N3D_FIRE_NO_WEAPON;
        return result;
    }

    if(n3d_player.weapon_jam)
    {
        result.kind = N3D_FIRE_JAMMED;
        return result;
    }

    uint8_t* ammo =
        N3D_RE_AmmoPoolForWeapon(n3d_player.active_weapon);
    if(!ammo)
    {
        result.kind = N3D_FIRE_UNRESOLVED;
        return result;
    }

    result.ammo_before = *ammo;
    result.ammo_after = *ammo;

    /*
     * Omnipotent bypasses normal ammo consumption. For ordinary firing, an
     * empty pool rejects the shot.
     */
    if(!n3d_player.omnipotent && *ammo == 0)
    {
        result.kind = N3D_FIRE_NO_AMMO;
        return result;
    }

    if(N3D_RE_WeaponUsesProjectile(n3d_player.active_weapon))
    {
        const int slot = N3D_RE_FirstFreeProjectileSlot();
        if(slot < 0)
        {
            result.kind = N3D_FIRE_PROJECTILE_POOL_FULL;
            return result;
        }

        /*
         * Pool availability is checked before ammo mutation, matching A97C
         * caller ordering recovered for projectile weapons.
         */
        if(!N3D_RE_InitializeProjectileFromAngle(
                slot,
                n3d_player.active_weapon,
                n3d_player.world_x,
                n3d_player.world_y,
                projectile_sequence_base,
                n3d_player.angle_degrees))
        {
            result.kind = N3D_FIRE_UNRESOLVED;
            return result;
        }

        result.projectile_slot = (int8_t)slot;
        result.kind = N3D_FIRE_PROJECTILE_READY;
    }
    else if(n3d_player.active_weapon == N3D_WEAPON_SILVER_PISTOL)
    {
        result.kind = N3D_FIRE_HITSCAN_READY;
    }
    else
    {
        result.kind = N3D_FIRE_UNRESOLVED;
        return result;
    }

    if(!n3d_player.omnipotent)
    {
        --(*ammo);
        result.ammo_consumed = 1;
    }

    result.ammo_after = *ammo;
    return result;
}
