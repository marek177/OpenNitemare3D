using System;
using System.Collections.Generic;

/*
Copyright (c) 2004-2007, Lode Vandevenne

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

namespace Nitemare3D
{
    public class Player : Entity
    {
        public int health = RecoveredRuntime.PlayerInitialHealth;
        public byte playerState = RecoveredRuntime.PlayerAliveState;
        public bool omnipotent;

        /// <summary>
        /// Applies the normal enemy/projectile damage receiver recovered at
        /// NITE3W.EXE seg3:8C09..8C98. Enemy-class damage production remains
        /// separate until the runtime classes can be bound without guessing.
        /// </summary>
        public bool ApplyVerifiedDamage(byte damage)
        {
            if(omnipotent || playerState == RecoveredRuntime.PlayerNormalDeathState)
            {
                return false;
            }

            if(damage >= health)
            {
                health = 0;
                playerState = RecoveredRuntime.PlayerNormalDeathState;
                return true;
            }

            health -= damage;
            return false;
        }

        /// <summary>
        /// Restores health while preserving the original observable 100-point
        /// ceiling. Pickup identity and pickup-specific amounts are intentionally
        /// left to their separately verified dispatch paths.
        /// </summary>
        public void RestoreVerifiedHealth(int amount)
        {
            if(amount <= 0 || health >= RecoveredRuntime.PlayerMaximumHealth)
            {
                return;
            }

            health = Math.Min(
                RecoveredRuntime.PlayerMaximumHealth,
                health + amount);
        }

        public Vec2 plane = new Vec2(0, .8f);

        float walkSpeed = 3;
        float runSpeed = 5;
        public float rotation = 0;

        const int weaponCount = 4;
        public int weaponIndex = -1;
        public PlayerWeapon[] weapons = new PlayerWeapon[4]
        {
            new PlayerPlasmaPistol(),
            new PlayerMagicWand(),
            new PlayerRevolver(),
            new PlayerAutoPistol()
        };

        BitmapImage wall;

        const int WallTextureSize = 64;

        int RayHeight = 152;
        int RayWidth = 304;
        int spriteCount = 0;
        const int maxSprites = 4096;
        public void AddSprite(ISprite sprite)
        {
            if(spriteCount == maxSprites)
            {
                throw new Exception("Sprite limit of " + maxSprites + " exceeded!");
            }

            sprites[spriteCount] = sprite;
            spriteCount++;
        }

        public override void Start()
        {
            weapons[0].hasWeapon = true;

            RayWidth = (int)(RayWidth * GameWindow.scale);
            RayHeight = (int)(RayHeight * GameWindow.scale);

            zBuffer = new float[RayWidth];
            hasCollision = false;
        }

        ISprite[] sprites = new ISprite[maxSprites];
        int[] spriteOrder = new int[maxSprites];
        float[] spriteDistance = new float[maxSprites];
        float[] zBuffer;

        class DecendingComparer<TKey> : IComparer<float>
        {
            public int Compare(float x, float y)
            {
                return y.CompareTo(x);
            }
        }

        void SortSprites()
        {
            SortedList<float, int> values = new SortedList<float, int>(new DecendingComparer<float>());

            int cnt = 0;
            for(int i = 0; i < spriteCount; i++)
            {
                // Prevent sprites at exactly the same distance from colliding in SortedList.
                if(values.ContainsKey(spriteDistance[i])) { continue; }
                cnt++;
                values.Add(spriteDistance[i], spriteOrder[i]);
            }

            for(int i = 0; i < cnt; i++)
            {
                spriteDistance[i] = values.Keys[i];
                spriteOrder[i] = values.Values[i];
            }
        }

        public Vec2 direction = new Vec2();

        bool lineLine(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
        {
            float uA = ((x4-x3)*(y1-y3) - (y4-y3)*(x1-x3)) / ((y4-y3)*(x2-x1) - (x4-x3)*(y2-y1));
            float uB = ((x2-x1)*(y1-y3) - (y2-y1)*(x1-x3)) / ((y4-y3)*(x2-x1) - (x4-x3)*(y2-y1));

            return uA >= 0 && uA <= 1 && uB >= 0 && uB <= 1;
        }

        bool CellHasBlockingEntity(int tileX, int tileY)
        {
            foreach(var entity in Entity.entities)
            {
                if(entity.id == id || !entity.hasCollision) { continue; }

                int entityX = (int)MathF.Floor(entity.position.X);
                int entityY = (int)MathF.Floor(entity.position.Y);
                if(entityX == tileX && entityY == tileY)
                {
                    return true;
                }
            }

            return false;
        }

        bool CanOccupy(float x, float y)
        {
            float half = RecoveredRuntime.PlayerCollisionHalfExtentTiles;
            int minX = (int)MathF.Floor(x - half);
            int maxX = (int)MathF.Floor(x + half);
            int minY = (int)MathF.Floor(y - half);
            int maxY = (int)MathF.Floor(y + half);

            for(int tx = minX; tx <= maxX; tx++)
            {
                for(int ty = minY; ty <= maxY; ty++)
                {
                    if(tx < 0 || tx >= RecoveredRuntime.MapWidth ||
                       ty < 0 || ty >= RecoveredRuntime.MapHeight)
                    {
                        return false;
                    }

                    Tile tile = Level.tilemap[tx, ty];
                    if(tile == null || tile.obstacle || CellHasBlockingEntity(tx, ty))
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        /// <summary>
        /// The original executable advances movement in one-world-unit major-axis
        /// steps and collision-tests X/Y separately. The C# port stores positions
        /// in tile units, so subdividing to at most 1/64 tile reproduces that
        /// granularity while preserving the port's floating-point movement model.
        /// </summary>
        void MoveWithRecoveredCollision(Vec2 delta)
        {
            float largestWorldDelta = MathF.Max(MathF.Abs(delta.X), MathF.Abs(delta.Y)) *
                                      RecoveredRuntime.WorldUnitsPerTile;
            int steps = Math.Max(1, (int)MathF.Ceiling(largestWorldDelta));
            float stepX = delta.X / steps;
            float stepY = delta.Y / steps;

            for(int i = 0; i < steps; i++)
            {
                if(stepX != 0 && CanOccupy(position.X + stepX, position.Y))
                {
                    position.X += stepX;
                }

                if(stepY != 0 && CanOccupy(position.X, position.Y + stepY))
                {
                    position.Y += stepY;
                }
            }
        }

        void HandleUse()
        {
            // NITE3W.EXE input mask 0x0200 is edge-triggered, not repeated while held.
            if(!Input.IsKeyPressed(KeyboardKey.Space)) { return; }

            int tx = (int)MathF.Floor(position.X);
            int ty = (int)MathF.Floor(position.Y);

            // The original chooses exactly one adjacent cardinal MAP cell. Diagonal
            // view octants are quantized to a cardinal neighbor.
            if(MathF.Abs(direction.X) >= MathF.Abs(direction.Y))
            {
                tx += direction.X >= 0 ? 1 : -1;
            }
            else
            {
                ty += direction.Y >= 0 ? 1 : -1;
            }

            if(tx < 0 || tx >= RecoveredRuntime.MapWidth ||
               ty < 0 || ty >= RecoveredRuntime.MapHeight)
            {
                return;
            }

            // USE classifies both the wall and object bytes in the selected MAP cell.
            Level.tilemap[tx, ty]?.OnUse();

            foreach(var entity in Entity.entities)
            {
                if((int)MathF.Floor(entity.position.X) == tx &&
                   (int)MathF.Floor(entity.position.Y) == ty)
                {
                    entity.SendMessage("OnUse");
                }
            }
        }

        public void RenderRaycaster()
        {
            bool flipped = false;

            for (int x = 0; x < RayWidth; x++)
            {
                float cameraX = 2 * x / (float)RayWidth - 1;

                float rayDirX = direction.X + plane.X * cameraX;
                float rayDirY = direction.Y + plane.Y * cameraX;

                int mapX = (int)position.X;
                int mapY = (int)position.Y;

                Vec2 sideDist = new Vec2();

                float deltaDistX = Math.Abs(1 / rayDirX);
                float deltaDistY = Math.Abs(1 / rayDirY);

                float perpWallDist;

                Vec2i step = new Vec2i();

                int hit = 0;
                int side = 0;

                if (rayDirX < 0)
                {
                    step.X = -1;
                    sideDist.X = (position.X - mapX) * deltaDistX;
                }
                else
                {
                    step.X = 1;
                    sideDist.X = (mapX + 1.0f - position.X) * deltaDistX;
                }
                if (rayDirY < 0)
                {
                    step.Y = -1;
                    sideDist.Y = (position.Y - mapY) * deltaDistY;
                }
                else
                {
                    step.Y = 1;
                    sideDist.Y = (mapY + 1.0f - position.Y) * deltaDistY;
                }

                while (hit == 0)
                {
                    if (sideDist.X < sideDist.Y)
                    {
                        sideDist.X += deltaDistX;
                        mapX += step.X;
                        side = 0;
                    }
                    else
                    {
                        sideDist.Y += deltaDistY;
                        mapY += step.Y;
                        side = 1;
                    }

                    if (mapX < 0 || mapY < 0) { hit = 1; break; }
                    if (mapX > 63 || mapY > 63) { hit = 1; break; }

                    var wall = Level.tilemap[mapX, mapY].textureID;
                    if (wall <= 149 && wall > -1) { hit = 1; }

                    if (hit == 1)
                    {
                        flipped = Level.tilemap[mapX, mapY].flip;
                        this.wall = Img.current.entries[wall];
                    }
                }

                if (side == 0) perpWallDist = (mapX - position.X + (1 - step.X) / 2) / rayDirX;
                else perpWallDist = (mapY - position.Y + (1 - step.Y) / 2) / rayDirY;

                int lineHeight = (int)(RayHeight / perpWallDist);

                int drawStart = -lineHeight / 2 + (int)RayHeight / 2;
                if (drawStart < 0) drawStart = 0;
                int drawEnd = lineHeight / 2 + (int)RayHeight / 2;
                if (drawEnd >= (int)RayHeight) drawEnd = (int)RayHeight - 1;

                var stepAmount = 1.0 * WallTextureSize / lineHeight;
                var texPos = (drawStart - (int)RayHeight / 2 + lineHeight / 2) * stepAmount;

                float wallX;
                if (side == 0) wallX = position.Y + perpWallDist * rayDirY;
                else wallX = position.X + perpWallDist * rayDirX;
                wallX -= (float)Math.Floor((wallX));

                int texX = (int)(wallX * 64);

                if (flipped && wall.width > 64)
                {
                    texX += 64;
                }

                for (int y = 0; y < drawStart; y++)
                {
                    GameWindow.frameBuffer[8 + x, 4 + y] = ImageConsts.PALETTE_CEILING;
                }

                if (drawEnd < RayHeight && drawEnd > 0)
                {
                    for (int y = drawEnd; y < RayHeight; y++)
                    {
                        GameWindow.frameBuffer[8 + x, 4 + y] = ImageConsts.PALETTE_FLOOR;
                    }
                }

                for (int y = drawStart; y < drawEnd; y++)
                {
                    int texY = (int)texPos & (WallTextureSize - 1);
                    texPos += stepAmount;

                    byte color = wall.data[texX, texY];
                    GameWindow.frameBuffer[8 + x, 4 + y] = color;
                }

                zBuffer[x] = perpWallDist;
            }

            for(int i = 0; i < spriteCount; i++)
            {
                spriteOrder[i] = i;
                spriteDistance[i] = Vec2.Distance(position, sprites[i].spritePosition);
            }

            SortSprites();

            for(int i = 0; i < spriteCount; i++)
            {
                Vec2 spritePos = sprites[spriteOrder[i]].spritePosition - position;

                var sprite = sprites[spriteOrder[i]];
                if(!sprite.visible) { continue; }

                var spriteW = Img.current.entries[sprite.spriteIndex].width;
                var spriteH = Img.current.entries[sprite.spriteIndex].height;

                float invDet = 1.0f / (plane.X * direction.Y - direction.X * plane.Y);

                float transformX = invDet * (direction.Y * spritePos.X - direction.X * spritePos.Y);
                float transformY = invDet * (-plane.Y * spritePos.X + plane.X * spritePos.Y);

                int spriteScreenX = (int)((RayWidth / 2) * (1 + transformX / transformY));
                float uDiv = (64f / spriteW);
                float vDiv = (64f / spriteH);
                float vMove = ((64-spriteH) - sprite.yOffset) * GameWindow.scale;

                int vMoveScreen = (int)(vMove / transformY);

                int spriteHeight = (int)(MathF.Abs(RayHeight / transformY) / vDiv);

                int drawStartY = -spriteHeight / 2 + RayHeight / 2 + vMoveScreen;
                if(drawStartY < 0) drawStartY = 0;
                int drawEndY = spriteHeight / 2 + RayHeight / 2 + vMoveScreen;
                if(drawEndY >= RayHeight) drawEndY = RayHeight - 1;

                int spriteWidth = (int)(MathF.Abs(RayHeight / transformY) / uDiv);
                int drawStartX = -spriteWidth / 2 + spriteScreenX;
                if(drawStartX < 0) drawStartX = 0;
                int drawEndX = spriteWidth / 2 + spriteScreenX;
                if(drawEndX >= RayWidth) drawEndX = RayWidth - 1;

                for(int stripe = drawStartX; stripe < drawEndX; stripe++)
                {
                    int texX = (int)(256 * (stripe - (-spriteWidth / 2 + spriteScreenX)) * spriteW / spriteWidth) / 256;

                    if(transformY > 0 && stripe > 0 && stripe < RayWidth && transformY < zBuffer[stripe])
                    {
                        for(int y = drawStartY; y < drawEndY; y++)
                        {
                            int d = (y-vMoveScreen) * 256 - RayHeight * 128 + spriteHeight * 128;
                            int texY = ((d * spriteH) / spriteHeight) / 256;
                            var color = Img.current.entries[sprite.spriteIndex].data[texX, texY];

                            if(color != 31)
                            {
                                GameWindow.frameBuffer[8 + stripe, 4 + y] = color;
                            }
                        }
                    }
                }
            }
        }

        void RenderWeapon()
        {
            if(weaponIndex == -1) { return; }
            GameWindow.DrawImg(weapons[weaponIndex].texture, ImageConsts.UI_WEAPONPOSITION);

            GameWindow.DrawImg(ImageConsts.UI_FACE_START, ImageConsts.UI_FACEPOSITION);

            var input = Input.GetNumberInput();
            int oldWeaponIndex = weaponIndex;
            if (input > 0 && input <= weaponCount)
            {
                fireTimer = 0;
                weaponIndex = input - 1;

                if(!weapons[weaponIndex].hasWeapon)
                {
                    weaponIndex = oldWeaponIndex;
                }
            }

            fireTimer += Time.dt;
            if (Input.IsKeyDown(KeyboardKey.LControl))
            {
                if (fireTimer > weapons[weaponIndex].fireTime)
                {
                    fireTimer = 0;
                    weapons[weaponIndex].Fire();
                    SoundEffect.PlaySound(weapons[weaponIndex].fireSound);
                }
            }
        }

        public Player()
        {
        }

        float fireTimer = 0;

        public void SetRotation(float angle)
        {
            float oldRot = rotation;
            float oldPlaneX = plane.X;

            rotation = angle * (3.14f / 180);
            plane.X = plane.X * (float)Math.Cos(rotation - oldRot) - plane.Y * (float)Math.Sin(rotation - oldRot);
            plane.Y = oldPlaneX * (float)Math.Sin(rotation - oldRot) + plane.Y * (float)Math.Cos(rotation - oldRot);
        }

        public override void Update()
        {
            float oldRot = rotation;

            if (Input.IsKeyDown(KeyboardKey.Right))
            {
                rotation += 3 * Time.dt;
            }

            if (Input.IsKeyDown(KeyboardKey.Left))
            {
                rotation -= 3 * Time.dt;
            }

            float oldPlaneX = plane.X;
            plane.X = plane.X * (float)Math.Cos(rotation - oldRot) - plane.Y * (float)Math.Sin(rotation - oldRot);
            plane.Y = oldPlaneX * (float)Math.Sin(rotation - oldRot) + plane.Y * (float)Math.Cos(rotation - oldRot);

            direction = new Vec2(MathF.Cos(rotation), MathF.Sin(rotation)).Normalize();

            if (Input.IsKeyDown(KeyboardKey.Up))
            {
                MoveWithRecoveredCollision(direction * (Time.dt * walkSpeed));
            }

            if (Input.IsKeyDown(KeyboardKey.Down))
            {
                MoveWithRecoveredCollision(direction * (-Time.dt * walkSpeed));
            }

            HandleUse();
            RenderRaycaster();
            RenderWeapon();
        }
    }
}
