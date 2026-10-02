using System;

namespace Nitemare3D
{
    public enum AutomapPaletteProfile
    {
        DOS,
        Win16
    }

    // Reverse-engineered Nitemare-3D automap core.
    // Canonical request model matches DOS v2.0 / Win16; legacy DOS v1.0/v1.7/v1.9
    // uses the same behavior with requests 4..8 shifted down by one.
    public static class Automap
    {
        const int MapSize = 64;
        const int ViewWidth = 62;
        const int ViewHeight = 36;
        const int ViewScreenX = 256;
        const int ViewScreenY = 162;

        const int MagicGaugeX = 211;
        const int CrystalGaugeX = 233;
        const int GaugeY = 181;
        const int GaugeWidth = 18;
        const int GaugeHeight = 7;

        static readonly byte[] cells = new byte[MapSize * MapSize];
        static readonly byte[] logicalColors = new byte[16];
        static readonly Random noiseRandom = new Random(0x4E3344);

        static bool colorsReady;
        static bool playerBlink;
        static bool previousF9;
        static bool previousF10;

        static double slowAccumulator;
        static uint slowTick;

        static int viewX;
        static int viewY;
        static int damageFlashTimer = -1;

        public static AutomapPaletteProfile PaletteProfile { get; set; } = AutomapPaletteProfile.Win16;

        public static int MagicEyePower { get; private set; }
        public static int CrystalBallPower { get; private set; }

        public static bool MagicEyeActive { get; private set; }
        public static bool CrystalBallActive { get; private set; }

        public static void Reset()
        {
            Array.Clear(cells, 0, cells.Length);
            MagicEyePower = 0;
            CrystalBallPower = 0;
            MagicEyeActive = false;
            CrystalBallActive = false;
            playerBlink = false;
            previousF9 = false;
            previousF10 = false;
            slowAccumulator = 0;
            slowTick = 0;
            damageFlashTimer = -1;
            colorsReady = false;
        }

        static int Index(int x, int y)
        {
            return x * MapSize + y;
        }

        static bool InsideMap(int x, int y)
        {
            return x >= 0 && y >= 0 && x < MapSize && y < MapSize;
        }

        static void EnsureColors()
        {
            if (colorsReady)
                return;

            for (int i = 0; i < logicalColors.Length; i++)
                logicalColors[i] = ResolveLogicalColorSlow(i);

            colorsReady = true;
        }

        static byte ResolveLogicalColorSlow(int logical)
        {
            int targetR;
            int targetG;
            int targetB;

            if (logical == 6)
            {
                targetR = 42 * 4;
                targetG = 21 * 4;
                targetB = 0;
            }
            else if (logical < 8)
            {
                targetR = ((logical & 4) != 0) ? 42 * 4 : 0;
                targetG = ((logical & 2) != 0) ? 42 * 4 : 0;
                targetB = ((logical & 1) != 0) ? 42 * 4 : 0;
            }
            else
            {
                targetR = ((logical & 4) != 0) ? 63 * 4 : 21 * 4;
                targetG = ((logical & 2) != 0) ? 63 * 4 : 21 * 4;
                targetB = ((logical & 1) != 0) ? 63 * 4 : 21 * 4;
            }

            int start = PaletteProfile == AutomapPaletteProfile.DOS ? 0 : 10;
            int end = PaletteProfile == AutomapPaletteProfile.DOS ? 256 : 246;

            int bestIndex = start;
            int bestDistance = int.MaxValue;

            for (int i = start; i < end; i++)
            {
                int p = i * 3;
                if (p + 2 >= GameWindow.pal.Length)
                    break;

                int dr = GameWindow.pal[p] - targetR;
                int dg = GameWindow.pal[p + 1] - targetG;
                int db = GameWindow.pal[p + 2] - targetB;
                int distance = dr * dr + dg * dg + db * db;

                if (distance < bestDistance)
                {
                    bestDistance = distance;
                    bestIndex = i;
                }
            }

            return (byte)bestIndex;
        }

        public static byte ResolveLogicalColor(int logical)
        {
            EnsureColors();
            return logicalColors[logical & 0x0F];
        }

        public static void SetPaletteProfile(AutomapPaletteProfile profile)
        {
            if (PaletteProfile == profile)
                return;

            PaletteProfile = profile;
            colorsReady = false;
        }

        static bool IsDynamicDoor(Tile tile)
        {
            if (tile == null)
                return false;

            if (tile.door || tile is Curtain)
                return true;

            string name = tile.type.ToString();
            return name.IndexOf("door", StringComparison.OrdinalIgnoreCase) >= 0 ||
                   name.IndexOf("curtain", StringComparison.OrdinalIgnoreCase) >= 0;
        }

        // Current OpenNitemare3D renderer is tile/DDA based, not the original VEC renderer.
        // This records the visible hit cell with the correct original logical color.
        public static void DiscoverWall(int x, int y, Tile tile)
        {
            if (!InsideMap(x, y) || tile == null)
                return;

            cells[Index(x, y)] = ResolveLogicalColor(IsDynamicDoor(tile) ? 9 : 2);
        }

        public static void DoorOpened(int x, int y)
        {
            if (InsideMap(x, y))
                cells[Index(x, y)] = 0;
        }

        public static void DoorClosed(int x, int y)
        {
            if (!InsideMap(x, y))
                return;

            Tile tile = Level.tilemap[x, y];
            cells[Index(x, y)] = ResolveLogicalColor(IsDynamicDoor(tile) ? 9 : 2);
        }

        public static bool TryAddMagicEye(int amount = 20)
        {
            if (MagicEyePower >= 100)
                return false;

            MagicEyePower += amount;
            if (MagicEyePower > 100)
                MagicEyePower = 100;
            return true;
        }

        public static bool TryAddCrystalBall(int amount = 20)
        {
            if (CrystalBallPower >= 100)
                return false;

            CrystalBallPower += amount;
            if (CrystalBallPower > 100)
                CrystalBallPower = 100;
            return true;
        }

        public static void TriggerDamageFlash()
        {
            // DOS v2.0 / Win16 behavior: 3 presentation updates.
            damageFlashTimer = 3;
        }

        public static void ClearPlayerCellForSave()
        {
            if (Game.player == null)
                return;

            int x = (int)Game.player.position.X;
            int y = (int)Game.player.position.Y;
            if (InsideMap(x, y))
                cells[Index(x, y)] = 0;
        }

        public static byte[] CopyPersistentMap()
        {
            var result = new byte[cells.Length];
            Array.Copy(cells, result, cells.Length);
            return result;
        }

        public static void LoadPersistentMap(byte[] data)
        {
            Array.Clear(cells, 0, cells.Length);
            if (data == null)
                return;

            Array.Copy(data, cells, Math.Min(data.Length, cells.Length));
        }

        static void ToggleInput()
        {
            bool f9 = Input.IsKeyDown(KeyboardKey.F9);
            bool f10 = Input.IsKeyDown(KeyboardKey.F10);

            if (f9 && !previousF9 && MagicEyePower != 0)
                MagicEyeActive = !MagicEyeActive;

            if (f10 && !previousF10 && CrystalBallPower != 0)
                CrystalBallActive = !CrystalBallActive;

            previousF9 = f9;
            previousF10 = f10;
        }

        static void UpdatePower()
        {
            // Original nominal slow scheduler is 8 Hz. The phase is global and continues
            // even when the power is inactive, so first depletion after activation can be short.
            slowAccumulator += Time.dt;
            while (slowAccumulator >= 0.125)
            {
                slowAccumulator -= 0.125;
                slowTick++;

                if (MagicEyeActive && ((slowTick & 0x0F) == 0))
                    MagicEyePower--;

                if (CrystalBallActive && ((slowTick & 7) == 0))
                    CrystalBallPower--;

                if (MagicEyeActive && MagicEyePower <= 0)
                {
                    MagicEyePower = 0;
                    MagicEyeActive = false;
                }

                if (CrystalBallActive && CrystalBallPower <= 0)
                {
                    CrystalBallPower = 0;
                    CrystalBallActive = false;
                }
            }
        }

        public static void Update()
        {
            ToggleInput();
            UpdatePower();
        }

        static void UpdateViewOrigin()
        {
            if (Game.player == null)
                return;

            int px = (int)Game.player.position.X;
            int py = (int)Game.player.position.Y;

            viewX = Math.Clamp(px - 31, 0, 2);
            viewY = Math.Clamp(py - 18, 0, 28);
        }

        static void PutLogicalPixel(int x, int y, byte color)
        {
            float scale = GameWindow.scale;
            int x0 = (int)(x * scale);
            int y0 = (int)(y * scale);
            int x1 = (int)((x + 1) * scale);
            int y1 = (int)((y + 1) * scale);

            if (x1 <= x0) x1 = x0 + 1;
            if (y1 <= y0) y1 = y0 + 1;

            x0 = Math.Max(0, x0);
            y0 = Math.Max(0, y0);
            x1 = Math.Min((int)GameWindow.width, x1);
            y1 = Math.Min((int)GameWindow.height, y1);

            for (int px = x0; px < x1; px++)
                for (int py = y0; py < y1; py++)
                    GameWindow.frameBuffer[px, py] = color;
        }

        static void FillLogicalRect(int x, int y, int width, int height, byte color)
        {
            for (int py = 0; py < height; py++)
                for (int px = 0; px < width; px++)
                    PutLogicalPixel(x + px, y + py, color);
        }

        static void ClearVisible()
        {
            FillLogicalRect(ViewScreenX, ViewScreenY, ViewWidth, ViewHeight, ResolveLogicalColor(0));
        }

        static void DrawBaseAndPlayer()
        {
            UpdateViewOrigin();

            int playerX = (int)Game.player.position.X;
            int playerY = (int)Game.player.position.Y;

            if (InsideMap(playerX, playerY))
            {
                cells[Index(playerX, playerY)] =
                    playerBlink ? ResolveLogicalColor(15) : ResolveLogicalColor(0);
                playerBlink = !playerBlink;
            }

            for (int x = 0; x < ViewWidth; x++)
            {
                for (int y = 0; y < ViewHeight; y++)
                {
                    byte color = cells[Index(viewX + x, viewY + y)];
                    PutLogicalPixel(ViewScreenX + x, ViewScreenY + y, color);
                }
            }
        }

        static void DrawEnemies()
        {
            byte color = ResolveLogicalColor(12);

            foreach (Entity entity in Entity.entities)
            {
                if (!(entity is Guard guard) || !guard.visible)
                    continue;

                int x = (int)guard.position.X - viewX;
                int y = (int)guard.position.Y - viewY;

                if (x < 0 || y < 0 || x >= ViewWidth || y >= ViewHeight)
                    continue;

                PutLogicalPixel(ViewScreenX + x, ViewScreenY + y, color);
            }
        }

        static void DrawHeading()
        {
            if (Game.player == null)
                return;

            float px = Game.player.position.X;
            float py = Game.player.position.Y;
            float dx = Game.player.direction.X;
            float dy = Game.player.direction.Y;

            float bestT = float.MaxValue;

            if (dx > 0.0001f)
                bestT = Math.Min(bestT, (viewX + ViewWidth - 1 - px) / dx);
            else if (dx < -0.0001f)
                bestT = Math.Min(bestT, (viewX - px) / dx);

            if (dy > 0.0001f)
                bestT = Math.Min(bestT, (viewY + ViewHeight - 1 - py) / dy);
            else if (dy < -0.0001f)
                bestT = Math.Min(bestT, (viewY - py) / dy);

            if (bestT == float.MaxValue || bestT < 0)
                return;

            int x = (int)Math.Round(px + dx * bestT) - viewX;
            int y = (int)Math.Round(py + dy * bestT) - viewY;
            x = Math.Clamp(x, 0, ViewWidth - 1);
            y = Math.Clamp(y, 0, ViewHeight - 1);

            PutLogicalPixel(ViewScreenX + x, ViewScreenY + y, ResolveLogicalColor(14));
        }

        static void DrawNoise()
        {
            int p = MagicEyePower;
            if (p <= 0)
                return;

            int count = 500 / (p * p * p);
            byte color = ResolveLogicalColor(15);

            for (int i = 0; i < count; i++)
            {
                int x = noiseRandom.Next(ViewWidth);
                int y = noiseRandom.Next(ViewHeight);
                PutLogicalPixel(ViewScreenX + x, ViewScreenY + y, color);
            }
        }

        static void DrawPowerGauge(int x, int power, bool active, bool magicEye)
        {
            power = Math.Clamp(power, 0, 100);
            int width = power * GaugeWidth / 100;

            byte black = ResolveLogicalColor(0);
            byte color;

            if (magicEye)
            {
                color = active
                    ? ResolveLogicalColor(10)
                    : unchecked((byte)(ResolveLogicalColor(2) - 5));
            }
            else
            {
                color = active ? ResolveLogicalColor(12) : ResolveLogicalColor(4);
            }

            FillLogicalRect(x, GaugeY, GaugeWidth, GaugeHeight, black);
            if (width > 0)
                FillLogicalRect(x, GaugeY, width, GaugeHeight, color);
        }

        static void DrawPowerGauges()
        {
            DrawPowerGauge(MagicGaugeX, MagicEyePower, MagicEyeActive, true);
            DrawPowerGauge(CrystalGaugeX, CrystalBallPower, CrystalBallActive, false);
        }

        public static void Render()
        {
            EnsureColors();
            DrawPowerGauges();

            if (Game.player == null)
                return;

            if (damageFlashTimer >= 0)
            {
                damageFlashTimer--;
                if (damageFlashTimer >= 0)
                    FillLogicalRect(ViewScreenX, ViewScreenY, ViewWidth, ViewHeight, ResolveLogicalColor(12));
                else
                    ClearVisible();
                return;
            }

            if (!MagicEyeActive && !CrystalBallActive)
                return;

            UpdateViewOrigin();

            if (MagicEyeActive)
                DrawBaseAndPlayer();

            if (CrystalBallActive)
            {
                if (!MagicEyeActive)
                    ClearVisible();

                if (CrystalBallPower > 15 || ((CrystalBallPower & 1) != 0))
                    DrawEnemies();
            }

            DrawHeading();

            if (MagicEyeActive && MagicEyePower <= 15)
                DrawNoise();
        }
    }
}
