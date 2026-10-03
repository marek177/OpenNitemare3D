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
        Vec2 direction;

        public Projectile(Vec2 direction, ProjectileType type)
            : this(
                direction,
                type,
                type == ProjectileType.Magic
                    ? OriginalWeaponSelector.MagicWand
                    : OriginalWeaponSelector.SingleShotLaser)
        {
        }

        public Projectile(
            Vec2 direction,
            ProjectileType type,
            OriginalWeaponSelector weaponSelector)
        {
            this.direction = direction;
            this.type = type;
            this.weaponSelector = weaponSelector;
            anim.LoadAnimation(animations[(int)type]);
            Game.player.AddSprite(this);
            hasCollision = false;
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
                Entity.Remove(this);
                visible = false;
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
                    Entity.Remove(this);
                    visible = false;
                    return;
                }
            }

            if(!Level.IsWalkable((int)position.X, (int)position.Y, this))
            {
                delete = true;
            }

            if(delete){Entity.Remove(this); visible = false;}

        }
    }
}