using System;
using System.IO;

namespace Nitemare3D
{
    /// <summary>
    /// Win16 MAP.N header + selected raw 64x64 level.
    ///
    /// Layout recovered from FUN_1010_498A/FUN_1010_4868:
    ///   +0000 WORD level count
    ///   +0002 256-byte raw wall id -> wall runtime class table
    ///   +0102 256-byte raw object id -> object runtime class table
    ///   +0202 level 0, 0x2000 bytes
    ///   +2202 level 1, 0x2000 bytes
    ///   ...
    ///
    /// Each map cell is exactly two bytes: raw wall id, raw object id.
    /// The raw ids remain important for rendering/orientation. Runtime flags are
    /// derived from the translated classes, exactly like FUN_24BC/FUN_2556.
    /// </summary>
    public sealed class OriginalMapTables
    {
        public const int HeaderBytes = 0x202;
        public const int LevelBytes = 0x2000;

        // Derived wall-property bits from FUN_1010_24BC.
        public const byte WallAnyBlockingFamily = 0x02;
        public const byte WallHardBlock = 0x04;
        public const byte WallDynamicDoor = 0x08;
        public const byte WallClass2E2F = 0x10;
        public const byte WallScriptTouch = 0x40;

        // Derived object-property bits from FUN_1010_2556.
        public const byte ObjectRuntimePresent = 0x01;
        public const byte ObjectBlocksMovementOrLos = 0x02;
        public const byte ObjectSpecialTouch = 0x04;
        public const byte ObjectCreatesGuard = 0x08;
        public const byte ObjectLosPassThroughException = 0x20;
        public const byte ObjectClass04Special = 0x40;

        public ushort LevelCount { get; private set; }
        public int LevelIndex { get; private set; }

        public readonly byte[] WallClass = new byte[256];
        public readonly byte[] ObjectClass = new byte[256];
        public readonly byte[] WallProperty = new byte[256];
        public readonly byte[] ObjectProperty = new byte[256];

        public readonly byte[,] WallId =
            new byte[OriginalRuntime.MapWidth, OriginalRuntime.MapHeight];

        public readonly byte[,] ObjectId =
            new byte[OriginalRuntime.MapWidth, OriginalRuntime.MapHeight];

        public static OriginalMapTables Load(string path, int levelIndex)
        {
            byte[] bytes = File.ReadAllBytes(path);
            return Parse(bytes, levelIndex);
        }

        public static OriginalMapTables Parse(byte[] bytes, int levelIndex)
        {
            if (bytes == null)
                throw new ArgumentNullException(nameof(bytes));
            if (bytes.Length < HeaderBytes)
                throw new InvalidDataException("MAP file is shorter than 0x202-byte header.");

            var result = new OriginalMapTables
            {
                LevelIndex = levelIndex,
                LevelCount = (ushort)(bytes[0] | (bytes[1] << 8))
            };

            if (levelIndex < 0 || levelIndex >= result.LevelCount)
            {
                throw new ArgumentOutOfRangeException(
                    nameof(levelIndex),
                    "MAP level index is outside the header level count.");
            }

            Array.Copy(bytes, 0x0002, result.WallClass, 0, 256);
            Array.Copy(bytes, 0x0102, result.ObjectClass, 0, 256);

            BuildWallProperties(result.WallClass, result.WallProperty);
            BuildObjectProperties(result.ObjectClass, result.ObjectProperty);

            int levelOffset = HeaderBytes + levelIndex * LevelBytes;
            if (levelOffset > bytes.Length - LevelBytes)
                throw new InvalidDataException("MAP file does not contain the selected level.");

            for (int cell = 0; cell < 64 * 64; cell++)
            {
                int x = cell & 63;
                int y = cell >> 6;
                int offset = levelOffset + cell * 2;

                result.WallId[x, y] = bytes[offset + 0];
                result.ObjectId[x, y] = bytes[offset + 1];
            }

            return result;
        }

        public bool TryGetObjectClassAndVariant(
            byte rawObjectId,
            out byte objectClass,
            out byte variant)
        {
            objectClass = ObjectClass[rawObjectId];

            for (int i = 0; i <= rawObjectId; i++)
            {
                if (ObjectClass[i] == objectClass)
                {
                    variant = (byte)(rawObjectId - i);
                    return true;
                }
            }

            variant = 0;
            return false;
        }

        public bool TryFindObjectIdByClass(
            byte objectClass,
            byte startId,
            out byte rawObjectId)
        {
            for (int i = startId; i < 256; i++)
            {
                if (ObjectClass[i] == objectClass)
                {
                    rawObjectId = (byte)i;
                    return true;
                }
            }

            rawObjectId = 0;
            return false;
        }

        public byte WallClassAt(int x, int y)
        {
            return WallClass[WallId[x, y]];
        }

        public byte ObjectClassAt(int x, int y)
        {
            return ObjectClass[ObjectId[x, y]];
        }

        public byte WallPropertyAt(int x, int y)
        {
            return WallProperty[WallId[x, y]];
        }

        public byte ObjectPropertyAt(int x, int y)
        {
            return ObjectProperty[ObjectId[x, y]];
        }

        /// <summary>
        /// Literal semantic port of FUN_1010_24BC.
        /// </summary>
        public static void BuildWallProperties(
            byte[] wallClass,
            byte[] wallProperty)
        {
            if (wallClass == null || wallProperty == null ||
                wallClass.Length < 256 || wallProperty.Length < 256)
            {
                throw new ArgumentException(
                    "Wall class/property arrays must have 256 entries.");
            }

            for (int i = 0; i < 256; i++)
            {
                int c = wallClass[i];
                byte p = 0;

                if (c >= 0x01 && c <= 0x30)
                    p |= WallHardBlock;

                if (c >= 0x2E && c <= 0x2F)
                    p |= WallClass2E2F;

                if (c >= 0x31 && c <= 0x40)
                    p |= WallDynamicDoor;

                // Original bit 0 is derived from either hard-wall or door family.
                if ((p & 0x0C) != 0)
                    p |= 0x01;

                if (c >= 0x01 && c <= 0x40)
                    p |= WallAnyBlockingFamily;

                if (c >= 0x47 && c <= 0x48)
                    p |= WallScriptTouch;

                wallProperty[i] = p;
            }
        }

        /// <summary>
        /// Literal semantic port of FUN_1010_2556.
        /// </summary>
        public static void BuildObjectProperties(
            byte[] objectClass,
            byte[] objectProperty)
        {
            if (objectClass == null || objectProperty == null ||
                objectClass.Length < 256 || objectProperty.Length < 256)
            {
                throw new ArgumentException(
                    "Object class/property arrays must have 256 entries.");
            }

            for (int i = 0; i < 256; i++)
            {
                int c = objectClass[i];
                byte p = 0;

                if (c >= 0x06 && c <= 0x3D)
                    p |= ObjectRuntimePresent;

                if (c >= 0x08 && c <= 0x2D)
                    p |= ObjectBlocksMovementOrLos;

                if (c >= 0x2F && c <= 0x3D)
                    p |= ObjectSpecialTouch;

                if (c >= 0x08 && c <= 0x25)
                    p |= ObjectCreatesGuard;

                if (c == 0x2A)
                    p |= ObjectLosPassThroughException;

                if (c == 0x04)
                    p |= ObjectClass04Special;

                objectProperty[i] = p;
            }
        }

        /// <summary>
        /// FUN_1010_1394 door-target selection used by GUARD strategy 1.
        /// The original scans the DOOR pool in map creation order, compares
        /// Manhattan tile distance, and accepts a nearer candidate only if D50A
        /// can trace to it with secondary object checks disabled.
        /// </summary>
        public bool TryFindNearestReachableDoor(
            int startTileX,
            int startTileY,
            Func<int, int, bool> dynamicDoorAllowsSight,
            out int doorTileX,
            out int doorTileY)
        {
            doorTileX = 0;
            doorTileY = 0;
            int bestDistance = 0x7FFF;
            bool found = false;

            for (int y = 0; y < OriginalRuntime.MapHeight; y++)
            {
                for (int x = 0; x < OriginalRuntime.MapWidth; x++)
                {
                    if ((WallPropertyAt(x, y) & WallDynamicDoor) == 0)
                        continue;

                    int dx = x - startTileX;
                    int dy = y - startTileY;
                    int distance = Math.Abs(dx) + Math.Abs(dy);

                    if (distance >= bestDistance)
                        continue;

                    bool reachable = OriginalGuardDispatcher.TraceGuardGridLine(
                        startTileX,
                        startTileY,
                        dx,
                        dy,
                        distance,
                        false,
                        (cx, cy, secondary) =>
                            IsPerceptionIntermediateBlocked(
                                cx,
                                cy,
                                secondary,
                                dynamicDoorAllowsSight));

                    if (!reachable)
                        continue;

                    bestDistance = distance;
                    doorTileX = x;
                    doorTileY = y;
                    found = true;
                }
            }

            return found;
        }

        /// <summary>
        /// Cell-blocking portion of FUN_1010_D50A after Bresenham stepping.
        /// Dynamic-door state is supplied separately because the current port does
        /// not yet mirror the original 22-byte DOOR runtime record/state 0..4.
        /// </summary>
        public bool IsPerceptionIntermediateBlocked(
            int x,
            int y,
            bool secondaryCellChecks,
            Func<int, int, bool> dynamicDoorAllowsSight)
        {
            if (x < 0 || y < 0 ||
                x >= OriginalRuntime.MapWidth ||
                y >= OriginalRuntime.MapHeight)
            {
                return true;
            }

            byte wallFlags = WallPropertyAt(x, y);

            if ((wallFlags & WallAnyBlockingFamily) != 0)
            {
                if ((wallFlags & WallHardBlock) != 0)
                    return true;

                if ((wallFlags & WallDynamicDoor) != 0)
                {
                    if (dynamicDoorAllowsSight == null ||
                        !dynamicDoorAllowsSight(x, y))
                    {
                        return true;
                    }
                }
            }

            if (secondaryCellChecks)
            {
                byte objectFlags = ObjectPropertyAt(x, y);

                if ((objectFlags & ObjectBlocksMovementOrLos) != 0 &&
                    (objectFlags & ObjectLosPassThroughException) == 0)
                {
                    return true;
                }
            }

            return false;
        }
    }
}
