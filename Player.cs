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
        public int health = 100;


        public Vec2 plane = new Vec2(0, .8f);

        float walkSpeed = 3;
        float runSpeed = 5;
        public float rotation = 0;
        // DAT_1048_4C1C: persistent AREA id from wall class 0x44 markers.
        public byte areaWakeSelector = 0;

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
            // FUN_BA16 clears the player gameplay block, leaving no active or
            // owned weapon and zeroing all three ammo pools.
            OriginalRuntimeState.WeaponRuntime.ResetNewGame();
            OriginalRuntimeState.PickupRuntime.ResetNewGame();
            weaponIndex = -1;
            for (int i = 0; i < weapons.Length; i++)
                weapons[i].hasWeapon = false;

            RayWidth  = (int)(RayWidth * GameWindow.scale);
            RayHeight  = (int)(RayHeight * GameWindow.scale);
            
            zBuffer = new float[RayWidth];
            hasCollision = false;
        }

        public bool AcquireWeapon(OriginalWeaponSelector selector)
        {
            int index = (int)selector;
            if (index < 0 || index >= weapons.Length)
                return false;

            OriginalRuntimeState.WeaponRuntime.GrantWeapon(selector);
            weapons[index].hasWeapon = true;
            weaponIndex = index;
            return true;
        }

        ISprite[] sprites = new ISprite[maxSprites];        
        int[] spriteOrder = new int[maxSprites];
        float[] spriteDistance = new float[maxSprites];
        float[] zBuffer;

        // Win16 0x58FE: 320 WORD per-column wall visibility values. While the
        // legacy DDA still draws walls, populate this buffer in the original
        // Q4 projection domain so runtime-backed sprites can use the recovered
        // CC7C/3F80 wall tests instead of float zBuffer comparisons.
        readonly ushort[] originalWallVisibilityQ4 =
            new ushort[OriginalRendererCore.ScreenWidth];

        sealed class OriginalSpriteQueueEntry
        {
            public ISprite Sprite;
            public Entity RuntimeEntity;
            public IOriginalSpriteProjectionSource ProjectionSource;
            public BitmapImage Frame;
            public OriginalObjectRecord RuntimeObject;
            public OriginalSpriteProjectionExact.ProjectedSprite Projected;
        }

        readonly OriginalSpriteQueueEntry[] originalSpriteQueue =
            new OriginalSpriteQueueEntry[
                OriginalProjectedSpriteQueue.SlotCount];

        readonly bool[] originalSpriteQueueOccupied =
            new bool[
                OriginalProjectedSpriteQueue.SlotCount];

        readonly bool[] originalSpriteHandled =
            new bool[maxSprites];


        class DecendingComparer<TKey>: IComparer<float>
        {
            public int Compare(float x, float y)
            {
                return y.CompareTo(x);
            }
        }

        void SortSprites() //this function was a pain in the ass
        {
            SortedList<float, int> values = new SortedList<float, int>(new DecendingComparer<float>());

            
            int cnt = 0;
            for(int i = 0; i < spriteCount; i++) {
                //prevents sprites that have the same distance from crashing the game
                //this issue should be rare unless the player is in the starting position
                if(values.ContainsKey(spriteDistance[i])){continue;} 
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

        bool lineLine(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4) {

            // calculate the direction of the lines
            float uA = ((x4-x3)*(y1-y3) - (y4-y3)*(x1-x3)) / ((y4-y3)*(x2-x1) - (x4-x3)*(y2-y1));
            float uB = ((x2-x1)*(y1-y3) - (y2-y1)*(x1-x3)) / ((y4-y3)*(x2-x1) - (x4-x3)*(y2-y1));

            // if uA and uB are between 0-1, lines are colliding
            if (uA >= 0 && uA <= 1 && uB >= 0 && uB <= 1) {
                return true;
            }
            return false;
        }
        bool ExactOriginalSpriteQueueEnabled =>
            OriginalRendererStage4.Enabled &&
            OriginalRuntimeState.ExactTrigQ10 != null &&
            Math.Abs(GameWindow.scale - 1.0f) <= 0.0001f;

        void BuildOriginalSpriteQueue()
        {
            Array.Clear(
                originalSpriteQueue,
                0,
                originalSpriteQueue.Length);

            Array.Clear(
                originalSpriteQueueOccupied,
                0,
                originalSpriteQueueOccupied.Length);

            Array.Clear(
                originalSpriteHandled,
                0,
                originalSpriteHandled.Length);

            if (!ExactOriginalSpriteQueueEnabled ||
                Img.current == null)
            {
                return;
            }

            short playerWorldX =
                (short)MathF.Round(
                    position.X *
                    OriginalRuntime.WorldUnitsPerTile);

            short playerWorldY =
                (short)MathF.Round(
                    position.Y *
                    OriginalRuntime.WorldUnitsPerTile);

            int angle =
                OriginalProjectileRuntime.AngleFromDirection(
                    direction.X,
                    direction.Y);

            for (int i = 0; i < spriteCount; i++)
            {
                ISprite sprite = sprites[i];

                if (sprite == null ||
                    !sprite.visible)
                {
                    continue;
                }

                Entity runtimeEntity =
                    sprite as Entity;

                IOriginalSpriteProjectionSource projectionSource =
                    sprite as IOriginalSpriteProjectionSource;

                OriginalObjectRecord runtimeObject;
                BitmapImage frame;

                if (projectionSource != null)
                {
                    // Runtime projectile semantics include the original +/-20
                    // projection-call threshold inside TryGetOriginalSpriteFrame.
                    originalSpriteHandled[i] = true;

                    if (!projectionSource.TryGetOriginalProjectionObject(
                            out runtimeObject) ||
                        !projectionSource.TryGetOriginalSpriteFrame(
                            out frame))
                    {
                        continue;
                    }
                }
                else if (runtimeEntity != null &&
                         OriginalRuntimeState.TryGetObjectRecord(
                             runtimeEntity,
                             out runtimeObject))
                {
                    int frameIndex =
                        runtimeObject.Component03;

                    if (frameIndex < 0 ||
                        !OriginalRuntimeState.ObjectDefinitions
                            .TryGetBitmapFrame(
                                runtimeObject.DefinitionId,
                                frameIndex,
                                Img.current.rawData,
                                out frame))
                    {
                        // Keep the old visual path if this entity has not yet
                        // been fully migrated to an original IMG sequence.
                        continue;
                    }

                    originalSpriteHandled[i] = true;
                }
                else
                {
                    continue;
                }

                if (!OriginalSpriteProjectionExact.TryProject(
                        runtimeObject,
                        frame,
                        playerWorldX,
                        playerWorldY,
                        angle,
                        OriginalRuntimeState.ExactTrigQ10,
                        out var projected))
                {
                    continue;
                }

                if (!OriginalProjectedSpriteQueue.PassesThreeColumnWallGate(
                        originalWallVisibilityQ4,
                        projected.Left,
                        projected.CenterX,
                        projected.Right,
                        projected.ProjectedYQ4))
                {
                    continue;
                }

                int slot =
                    OriginalProjectedSpriteQueue.FindFreeSlot(
                        originalSpriteQueueOccupied,
                        projected.BaselineRow);

                if (slot < 0)
                {
                    throw new InvalidOperationException(
                        "Too many original projected sprites on screen.");
                }

                originalSpriteQueueOccupied[slot] = true;
                originalSpriteQueue[slot] =
                    new OriginalSpriteQueueEntry
                    {
                        Sprite = sprite,
                        RuntimeEntity = runtimeEntity,
                        ProjectionSource = projectionSource,
                        Frame = frame,
                        RuntimeObject = runtimeObject,
                        Projected = projected
                    };

                short cacheRow =
                    (short)Math.Max(
                        short.MinValue,
                        Math.Min(
                            short.MaxValue,
                            projected.BaselineRow));

                if (projectionSource != null)
                {
                    projectionSource.RecordOriginalProjectedBaseRow(
                        cacheRow);
                }

                if (runtimeEntity != null)
                {
                    bool overlapsAimCenter =
                        projected.Left - 4 <
                            OriginalRuntime.ViewportCenterX &&
                        OriginalRuntime.ViewportCenterX <
                            projected.Right + 4;

                    OriginalRuntimeState.RecordGuardProjection(
                        runtimeEntity,
                        cacheRow,
                        overlapsAimCenter);
                }
            }
        }

        void DrawOriginalSpriteQueue()
        {
            if (!ExactOriginalSpriteQueueEnabled)
                return;

            for (int slot = 0;
                slot < originalSpriteQueue.Length;
                slot++)
            {
                OriginalSpriteQueueEntry entry =
                    originalSpriteQueue[slot];

                if (entry == null ||
                    entry.Frame == null)
                {
                    continue;
                }

                var projected =
                    entry.Projected;

                BitmapImage frame =
                    entry.Frame;

                int firstX =
                    Math.Max(
                        OriginalRendererCore.ViewLeft,
                        projected.Left);

                int lastX =
                    Math.Min(
                        OriginalRendererCore.ViewRight,
                        projected.Right);

                int firstY =
                    Math.Max(
                        OriginalRendererCore.ViewTop,
                        projected.Top);

                int lastY =
                    Math.Min(
                        OriginalRendererCore.ViewBottom,
                        projected.Bottom);

                int screenHeight =
                    projected.Bottom -
                    projected.Top +
                    1;

                if (firstX > lastX ||
                    firstY > lastY ||
                    screenHeight <= 0)
                {
                    continue;
                }

                uint sourceStep16_16 =
                    OriginalProjectedSpriteQueue
                        .SpriteSourceStep16_16(
                            frame.height,
                            projected.Top,
                            projected.Bottom);

                bool bypassWall =
                    (entry.RuntimeObject.Flags & 0x10) != 0;

                for (int screenX = firstX;
                    screenX <= lastX;
                    screenX++)
                {
                    if (!OriginalProjectedSpriteQueue.ColumnPassesWall(
                            originalWallVisibilityQ4,
                            screenX,
                            projected.ProjectedYQ4,
                            bypassWall))
                    {
                        continue;
                    }

                    int texX =
                        OriginalProjectedSpriteQueue
                            .SpriteSourceCoordinate(
                                screenX,
                                projected.Left,
                                sourceStep16_16);

                    if (texX < 0 ||
                        texX >= frame.width)
                    {
                        continue;
                    }

                    for (int screenY = firstY;
                        screenY <= lastY;
                        screenY++)
                    {
                        int texY =
                            OriginalProjectedSpriteQueue
                                .SpriteSourceCoordinate(
                                    screenY,
                                    projected.Top,
                                    sourceStep16_16);

                        if (texY < 0 ||
                            texY >= frame.height)
                        {
                            continue;
                        }

                        byte color =
                            frame.data[
                                texX,
                                texY];

                        if (color !=
                            OriginalRuntime.TransparentPaletteIndex)
                        {
                            GameWindow.frameBuffer[
                                screenX,
                                screenY] = color;
                        }
                    }
                }
            }
        }

        public void RenderRaycaster()
        {
            OriginalRuntimeState.BeginRenderGeneration();
            Array.Clear(
                originalWallVisibilityQ4,
                0,
                originalWallVisibilityQ4.Length);

            bool originalWallsRendered =
                OriginalRendererStage4.TryRenderWalls(this);

            if (originalWallsRendered)
            {
                OriginalRendererStage4.CopyWallVisibilityQ4(
                    originalWallVisibilityQ4);
            }

            //var direction = new Vec2(MathF.Cos(rotation), MathF.Sin(rotation)).Normalize();





            bool flipped = false;

            if (!originalWallsRendered)
            {
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


                /*
                    side 0:
                                
                        #-#

                    side 1:

                        #
                        |
                        #
                */

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


                //DDA algirtrhtn
                float ThinCNT = 0;
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
                        // FUN_66B0 -> 65A6 updates animated/exploding VECs only
                        // after a visible wall span. The legacy DDA renderer is
                        // still the presentation path, so use its visible hit as
                        // the equivalent trigger for the recovered 0x2D lifecycle.
                        Level.UpdateOriginalExplodingWallVisibleAt(
                            mapX,
                            mapY,
                            OriginalRuntimeState.RuntimeClockMs);

                        flipped = Level.tilemap[mapX, mapY].flip;

                        var hitWall = Level.tilemap[mapX, mapY];
                        

                        

                        this.wall = Img.current.entries[wall];
                        

                        

                    }



                }



                //Calculate distance projected on camera direction (Euclidean distance will give fisheye effect!)
                if (side == 0) perpWallDist = (mapX - position.X + (1 - step.X) / 2) / rayDirX;
                else perpWallDist = (mapY - position.Y + (1 - step.Y) / 2) / rayDirY;

                //Calculate height of line to draw on screen
                int lineHeight = (int)(RayHeight / perpWallDist);

                //calculate lowest and highest pixel to fill in current stripe
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


                //draw ceiling
                for (int y = 0; y < drawStart; y++)
                {
                    GameWindow.frameBuffer[8 + x, 4 + y] = ImageConsts.PALETTE_CEILING;
                }


                if (drawEnd < RayHeight && drawEnd > 0)
                {
                    //draw floor
                    for (int y = drawEnd; y < RayHeight; y++)
                    {
                        GameWindow.frameBuffer[8 + x, 4 + y] = ImageConsts.PALETTE_FLOOR;
                    }
                }

                var tile = Level.tilemap[mapX,mapY];
                for (int y = drawStart; y < drawEnd; y++)
                {
                    int texY = (int)texPos & (WallTextureSize - 1);
                    texPos += stepAmount;

                    byte color = wall.data[texX, texY];
                    GameWindow.frameBuffer[8 + x, 4 + y] = color;




                }

                zBuffer[x] = perpWallDist;

                if (Math.Abs(GameWindow.scale - 1.0f) <= 0.0001f &&
                    perpWallDist > 0)
                {
                    int screenColumn =
                        OriginalRuntime.ViewportX + x;

                    if (screenColumn >= 0 &&
                        screenColumn < originalWallVisibilityQ4.Length)
                    {
                        originalWallVisibilityQ4[screenColumn] =
                            OriginalProjectedSpriteQueue
                                .WallVisibilityQ4FromPerpendicularDistance(
                                    perpWallDist);
                    }
                }


            }
            }

            BuildOriginalSpriteQueue();
            
            for(int i = 0; i < spriteCount; i++)
            {
                spriteOrder[i] = i;
                spriteDistance[i] = Vec2.Distance(position, sprites[i].spritePosition);
            }

            SortSprites();

            for(int i = 0; i < spriteCount; i++)
            {
                //sprite position relative to camera
                Vec2 spritePos = sprites[spriteOrder[i]].spritePosition - position;

                var sprite = sprites[spriteOrder[i]];
                if(!sprite.visible){continue;}

                // Do not mix the original VEC/visibility wall renderer with
                // the historical float-z sprite path. Stage 4 currently draws
                // only sprites that can enter the recovered 18-byte queue.
                if (originalWallsRendered)
                    continue;

                int originalSpriteIndex =
                    spriteOrder[i];

                if (ExactOriginalSpriteQueueEnabled &&
                    originalSpriteHandled[originalSpriteIndex])
                {
                    continue;
                }

                BitmapImage spriteFrame = null;

                if (sprite is IOriginalSpriteFrameSource originalFrameSource)
                {
                    // Runtime-backed sprites deliberately do not fall back to
                    // the historical flat spriteIndex list. A false return can
                    // mean "not projected this frame" in the original engine.
                    if (!originalFrameSource.TryGetOriginalSpriteFrame(
                            out spriteFrame))
                    {
                        continue;
                    }
                }
                else
                {
                    if (Img.current == null ||
                        sprite.spriteIndex < 0 ||
                        sprite.spriteIndex >= Img.current.entries.Count)
                    {
                        continue;
                    }

                    spriteFrame =
                        Img.current.entries[sprite.spriteIndex];
                }

                var spriteW = spriteFrame.width;
                var spriteH = spriteFrame.height;

                float invDet = 1.0f / (plane.X * direction.Y - direction.X * plane.Y);


                float transformX = invDet * (direction.Y * spritePos.X - direction.X * spritePos.Y);
                float transformY = invDet * (-plane.Y * spritePos.X + plane.X * spritePos.Y); 

                int spriteScreenX = (int)((RayWidth / 2) * (1 + transformX / transformY));
                float uDiv = (64f / spriteW);
                float vDiv = (64f / spriteH);
                float vMove = ((64-spriteH) - sprite.yOffset) * GameWindow.scale;

                int vMoveScreen = (int)(vMove / transformY);

                int spriteHeight = (int)(MathF.Abs(RayHeight / transformY) / vDiv);

                int rawDrawStartY = -spriteHeight / 2 + RayHeight / 2 + vMoveScreen;
                int rawDrawEndY = spriteHeight / 2 + RayHeight / 2 + vMoveScreen;
                int drawStartY = rawDrawStartY;
                if(drawStartY < 0) drawStartY = 0;
                int drawEndY = rawDrawEndY;
                if(drawEndY >= RayHeight) drawEndY = RayHeight - 1;


                int spriteWidth = (int)(MathF.Abs(RayHeight / transformY) / uDiv);
                int rawDrawStartX = -spriteWidth / 2 + spriteScreenX;
                int rawDrawEndX = spriteWidth / 2 + spriteScreenX;
                int drawStartX = rawDrawStartX;
                if(drawStartX < 0) drawStartX = 0;
                int drawEndX = rawDrawEndX;
                if(drawEndX >= RayWidth) drawEndX = RayWidth - 1; 


                bool projectedVisible = false;
                for(int stripe = drawStartX; stripe < drawEndX; stripe++)
                {
                    int texX = (int)(256 * (stripe - (-spriteWidth / 2 + spriteScreenX)) * spriteW / spriteWidth) / 256;


                    if(transformY > 0 && stripe > 0 && stripe < RayWidth && transformY < zBuffer[stripe])
                    {
                    projectedVisible = true;
                    for(int y = drawStartY; y < drawEndY; y++) //for every pixel of the current stripe
                    {
                        int d = (y-vMoveScreen) * 256 - RayHeight * 128 + spriteHeight * 128;
                        int texY = ((d * spriteH) / spriteHeight) / 256;
                        var color = spriteFrame.data[texX, texY];

                        byte transparentIndex =
                            sprite is IOriginalSpriteFrameSource
                            ? OriginalRuntime.TransparentPaletteIndex
                            : (byte)31;

                        if(color != transparentIndex)
                        {
                            GameWindow.frameBuffer[8 + stripe, 4 + y] = color;
                        }

                        

                    }
                    }
                }

                if (projectedVisible && sprite is Entity runtimeEntity)
                {
                    float scale = GameWindow.scale > 0 ? GameWindow.scale : 1f;

                    // The renderer works in a scaled viewport-local coordinate
                    // system; OBJECT+0x18 stores the original absolute screen row.
                    int projectedBaseRow =
                        OriginalRuntime.ViewportY +
                        (int)MathF.Round(rawDrawEndY / scale);

                    if (projectedBaseRow < short.MinValue)
                        projectedBaseRow = short.MinValue;
                    else if (projectedBaseRow > short.MaxValue)
                        projectedBaseRow = short.MaxValue;

                    int aimCenterLocal = RayWidth / 2;
                    int aimSlack = Math.Max(
                        1,
                        (int)MathF.Round(4f * scale));
                    bool overlapsAimCenter =
                        rawDrawStartX - aimSlack < aimCenterLocal &&
                        rawDrawEndX + aimSlack > aimCenterLocal;

                    if (sprite is IOriginalSpriteProjectionSource fallbackProjectionSource)
                    {
                        fallbackProjectionSource.RecordOriginalProjectedBaseRow(
                            (short)projectedBaseRow);
                    }

                    OriginalRuntimeState.RecordGuardProjection(
                        runtimeEntity,
                        (short)projectedBaseRow,
                        overlapsAimCenter);
                }
                

            }

            DrawOriginalSpriteQueue();
        }




        void RenderWeapon()
        {
            bool fireDown = Input.IsKeyDown(KeyboardKey.LControl);
            bool fireEdge = fireDown && !fireWasDown;
            fireWasDown = fireDown;

            var input = Input.GetNumberInput();
            if (input > 0 && input <= weaponCount)
            {
                var requested =
                    (OriginalWeaponSelector)(input - 1);

                if (OriginalRuntimeState.WeaponRuntime.TrySelect(requested))
                    weaponIndex = input - 1;
            }

            if (weaponIndex == -1)
                return; // original new-game state: empty hand

            GameWindow.DrawImg(
                weapons[weaponIndex].texture,
                ImageConsts.UI_WEAPONPOSITION);
            GameWindow.DrawImg(
                ImageConsts.UI_FACE_START,
                ImageConsts.UI_FACEPOSITION);

            var selector = (OriginalWeaponSelector)weaponIndex;
            if (!OriginalRuntimeState.WeaponRuntime.TryAcceptFireAttempt(
                    selector,
                    fireEdge,
                    fireDown))
            {
                return;
            }

            // Shot acceptance is deliberately after the cadence gate.
            // A rejected attempt keeps the original AA90 counter-reset behavior.
            if (OriginalRuntimeState.WeaponRuntime.Jammed)
                return;

            if (OriginalProjectileRuntime.WeaponUsesProjectile(
                    (byte)selector) &&
                OriginalRuntimeState.ProjectilePool.FirstFreeSlot() < 0)
            {
                return;
            }

            if (!OriginalRuntimeState.WeaponRuntime.ConsumeAmmo(selector))
                return;

            if (weapons[weaponIndex].Fire())
            {
                SoundEffect.PlaySound(weapons[weaponIndex].fireSound);
                OriginalRuntimeState.WakeGuardsAfterPlayerFire(
                    areaWakeSelector);
            }
        }


        public Player()
        {


        }
        bool fireWasDown = false;
        bool useWasDown = false;

        void MoveWithCollision(float amount)
        {
            var delta = direction * amount;
            WorldCollision.MovePlayerWithSliding(this, delta);
        }

        void UpdateAreaWakeSelector()
        {
            int tileX = (int)MathF.Floor(position.X);
            int tileY = (int)MathF.Floor(position.Y);

            if (Level.originalMap == null ||
                tileX < 0 || tileY < 0 ||
                tileX >= OriginalRuntime.MapWidth ||
                tileY >= OriginalRuntime.MapHeight)
            {
                return;
            }

            byte rawWallId = Level.originalMap.WallId[tileX, tileY];
            if (Level.originalMap.TryGetWallClassVariant(
                    rawWallId,
                    0x44,
                    out byte areaId))
            {
                // FUN_247A/8A20 preserve the previous AREA id off marker cells.
                areaWakeSelector = areaId;
                OriginalRuntimeState.SetPlayerAreaSelector(areaId);
            }
        }

        void UpdateUse()
        {
            bool useDown = Input.IsKeyDown(KeyboardKey.Space);
            if (useDown && !useWasDown)
            {
                UseDispatcher.TryUseAdjacent(this);
            }
            useWasDown = useDown;
        }

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
            direction = new Vec2(MathF.Cos(rotation), MathF.Sin(rotation)).Normalize();
            
            float oldRot = rotation;

            if (Input.IsKeyDown(KeyboardKey.Right))
            {
                rotation += 3 * Time.dt;
            }

            if (Input.IsKeyDown(KeyboardKey.Left))
            {
                rotation -= 3 * Time.dt;
            }

            if (Input.IsKeyDown(KeyboardKey.Up))
            {
                MoveWithCollision(Time.dt * walkSpeed);
            }

            if (Input.IsKeyDown(KeyboardKey.Down))
            {
                MoveWithCollision(-(Time.dt * walkSpeed));
            }

            float oldPlaneX = plane.X;

            plane.X = plane.X * (float)Math.Cos(rotation - oldRot) - plane.Y * (float)Math.Sin(rotation - oldRot);
            plane.Y = oldPlaneX * (float)Math.Sin(rotation - oldRot) + plane.Y * (float)Math.Cos(rotation - oldRot);

            UpdateAreaWakeSelector();
            UpdateUse();
            RenderRaycaster();
            RenderWeapon();


        }
    }
}