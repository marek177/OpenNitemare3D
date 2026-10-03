using System;
namespace Nitemare3D
{
    public enum ProjectileType
    {
        Plasma,
        Magic
    }
    public class Projectile : Entity, ISprite
    {


        static Animation[] animations = new Animation[]
        {
            new Animation(666, 2, 125), //plasma
            new Animation(671, 3, 125) //magic
        };

        AnimationHandler anim = new AnimationHandler();

        public int spriteIndex {get; set;}
        public bool visible{get;set;} = true;
        public Vec2 spritePosition{get;set;} = new Vec2();
        public float yOffset{get;set;}

        const float speed = 4;
        ProjectileType type;
        readonly OriginalWeaponSelector weaponSelector;
        readonly int runtimeSlotIndex;
        Vec2 direction;

        public Projectile(Vec2 direction, ProjectileType type)
            : this(
                direction,
                type,
                type == ProjectileType.Magic
                    ? OriginalWeaponSelector.MagicWand
                    : OriginalWeaponSelector.SingleShotLaser,
                -1)
        {
        }

        Projectile(
            Vec2 direction,
            ProjectileType type,
            OriginalWeaponSelector weaponSelector,
            int runtimeSlotIndex)
        {
            this.direction = direction;
            this.type = type;
            this.weaponSelector = weaponSelector;
            this.runtimeSlotIndex = runtimeSlotIndex;
            anim.LoadAnimation(animations[(int)type]);
            Game.player.AddSprite(this);
            hasCollision = false;
        }

        public static bool TrySpawn(
            Vec2 direction,
            ProjectileType type,
            OriginalWeaponSelector weaponSelector,
            Vec2 position)
        {
            short worldX = (short)MathF.Round(
                position.X * OriginalRuntime.WorldUnitsPerTile);
            short worldY = (short)MathF.Round(
                position.Y * OriginalRuntime.WorldUnitsPerTile);

            // Reserve the original eight-slot pool before creating the legacy
            // render/movement shell. Sequence base 0 is temporary until the
            // projectile IMG resource bridge owns presentation as well.
            if (!OriginalProjectileRuntime.TryAllocateAndInitialize(
                    OriginalRuntimeState.ProjectilePool.Slots,
                    (byte)weaponSelector,
                    worldX,
                    worldY,
                    0,
                    out int slotIndex))
            {
                return false;
            }

            var projectile = new Projectile(
                direction,
                type,
                weaponSelector,
                slotIndex);
            Entity.Add(projectile, position);
            return true;
        }

        void ReleaseRuntimeSlot()
        {
            if (runtimeSlotIndex < 0 ||
                runtimeSlotIndex >= OriginalRuntimeState.ProjectilePool.Slots.Length)
            {
                return;
            }

            OriginalRuntimeState.ProjectilePool.Slots[runtimeSlotIndex] = default;
        }

        void RemoveProjectile()
        {
            ReleaseRuntimeSlot();
            Entity.Remove(this);
            visible = false;
        }

        public override void Update()
        {
            anim.Update();
            position += direction * speed * Time.dt;
            spritePosition = position;
            spriteIndex = anim.index;

            bool delete = false;

            int projectileWorldX = (int)MathF.Round(
                position.X * OriginalRuntime.WorldUnitsPerTile);
            int projectileWorldY = (int)MathF.Round(
                position.Y * OriginalRuntime.WorldUnitsPerTile);

            if (OriginalRuntimeState.TryFindGuardHit(
                    projectileWorldX,
                    projectileWorldY,
                    out Entity hitGuard))
            {
                OriginalRuntimeState.ApplyPlayerWeaponDamage(
                    hitGuard,
                    weaponSelector,
                    false);
                RemoveProjectile();
                return;
            }

            
            foreach(var entity in entities)
            {
                if(entity.id == id || entity.id == Game.player.id){continue;}
                // Runtime-bound guards were already tested above using the
                // original +/-9 world-unit proximity rule.
                if (OriginalRuntimeState.TryGetGuardRecord(entity, out _))
                {
                    continue;
                }

                if(entity.position.Rounded().Equals(position.Rounded()) && entity.hasCollision)
                {
                    entity.SendMessage("ShootPlasma");
                    RemoveProjectile();
                    return;
                }
            }

            if(!Level.IsWalkable((int)position.X, (int)position.Y, this))
            {
                delete = true;
            }

            if(delete){RemoveProjectile();}

        }
    }
}