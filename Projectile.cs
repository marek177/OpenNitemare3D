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

            int angleDegrees =
                OriginalProjectileRuntime.AngleFromDirection(
                    direction.X,
                    direction.Y);

            if (!OriginalRuntimeState.TryGetProjectileDefinition(
                    weaponSelector,
                    false,
                    out byte flightDefinitionId,
                    out OriginalObjectDefinitionRecord flightDefinition))
            {
                return false;
            }

            int slotIndex =
                OriginalRuntimeState.ProjectilePool.FirstFreeSlot();
            if (slotIndex < 0)
                return false;

            ref var runtime =
                ref OriginalRuntimeState.ProjectilePool.Slots[slotIndex];

            OriginalProjectileRuntime.InitializeSpawnWithDefinition(
                ref runtime,
                flightDefinitionId,
                worldX,
                worldY,
                OriginalRuntimeState.RuntimeClockMs,
                flightDefinition.Interval);

            OriginalProjectileRuntime.ConfigureDdaFromAngle(
                ref runtime,
                angleDegrees);

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

        void RemoveRenderShell()
        {
            Entity.Remove(this);
            visible = false;
        }

        bool RuntimeCollisionAt(int worldX, int worldY)
        {
            if (OriginalRuntimeState.TryFindGuardHit(
                    worldX,
                    worldY,
                    out Entity hitGuard))
            {
                // Projectile damage deliberately does NOT require the current
                // render-generation stamp; the original collision route can
                // consume the cached OBJECT+0x18 projection row.
                OriginalRuntimeState.ApplyPlayerWeaponDamage(
                    hitGuard,
                    weaponSelector,
                    false);
                return true;
            }

            int tileX = worldX >> 6;
            int tileY = worldY >> 6;

            if (Level.originalMap == null ||
                tileX < 0 || tileY < 0 ||
                tileX >= OriginalRuntime.MapWidth ||
                tileY >= OriginalRuntime.MapHeight)
            {
                return true;
            }

            byte wallFlags =
                Level.originalMap.WallPropertyAt(tileX, tileY);

            if ((wallFlags & OriginalMapTables.WallHardBlock) != 0)
                return true;

            if ((wallFlags & OriginalMapTables.WallDynamicDoor) != 0)
            {
                if (!Level.TryGetOriginalDoorCollisionInfo(
                        tileX,
                        tileY,
                        out OriginalDoorCollisionInfo door))
                {
                    return true;
                }

                if (door.State != 0 && door.State != 4)
                    return true;
            }

            byte objectFlags =
                Level.originalMap.ObjectPropertyAt(tileX, tileY);

            // Actor-linked cells use the exact coordinate test above. Being in
            // the same 64-unit map cell is not itself a projectile hit.
            if ((objectFlags & OriginalMapTables.ObjectCreatesGuard) != 0)
                return false;

            // Class 0x2A is the recovered PERMEABLE exception: it carries the
            // ordinary occupancy bit together with 0x20, and projectile
            // traversal is allowed to continue through that combination.
            bool blockingObject =
                (objectFlags & OriginalMapTables.ObjectBlocksMovementOrLos) != 0;
            bool permeable =
                (objectFlags & OriginalMapTables.ObjectLosPassThroughException) != 0;

            return blockingObject && !permeable;
        }

        public override void Update()
        {
            anim.Update();
            spriteIndex = anim.index;

            if (runtimeSlotIndex < 0 ||
                runtimeSlotIndex >= OriginalRuntimeState.ProjectilePool.Slots.Length)
            {
                // Compatibility-only constructor: no recovered slot is bound.
                spritePosition = position;
                return;
            }

            ref var runtime =
                ref OriginalRuntimeState.ProjectilePool.Slots[runtimeSlotIndex];

            if (runtime.State == (byte)OriginalProjectileState.Free)
            {
                RemoveRenderShell();
                return;
            }

            if (OriginalRuntimeState.ProjectileLogicTickDue)
            {
                if (OriginalRuntimeState.ObjectDefinitions.TryGetHeader(
                        runtime.RenderObject.DefinitionId,
                        out var activeDefinition))
                {
                    OriginalProjectileRuntime.AdvanceAnimationIfDue(
                        ref runtime,
                        OriginalRuntimeState.RuntimeClockMs,
                        activeDefinition.FrameCount,
                        activeDefinition.Interval);
                }

                if (runtime.State == (byte)OriginalProjectileState.Free)
                {
                    RemoveRenderShell();
                    return;
                }

                if (runtime.State == (byte)OriginalProjectileState.Flying)
                {
                    bool collided =
                        OriginalProjectileRuntime.AdvanceTrajectoryAndCollide(
                            ref runtime,
                            OriginalRuntimeState.ProjectileSubstepsPerTick,
                            RuntimeCollisionAt);

                    position.X =
                        (float)runtime.RenderObject.WorldX /
                        OriginalRuntime.WorldUnitsPerTile;
                    position.Y =
                        (float)runtime.RenderObject.WorldY /
                        OriginalRuntime.WorldUnitsPerTile;

                    if (collided)
                    {
                        if (!OriginalRuntimeState.TryGetProjectileDefinition(
                                weaponSelector,
                                true,
                                out byte impactDefinitionId,
                                out OriginalObjectDefinitionRecord impactDefinition))
                        {
                            RemoveProjectile();
                            return;
                        }

                        OriginalProjectileRuntime.EnterImpactWithDefinition(
                            ref runtime,
                            impactDefinitionId,
                            OriginalRuntimeState.RuntimeClockMs,
                            impactDefinition.Interval);
                    }
                }
            }

            position.X =
                (float)runtime.RenderObject.WorldX /
                OriginalRuntime.WorldUnitsPerTile;
            position.Y =
                (float)runtime.RenderObject.WorldY /
                OriginalRuntime.WorldUnitsPerTile;
            spritePosition = position;
        }
    }
}