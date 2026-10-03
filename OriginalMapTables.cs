using System;
using System.Collections.Generic;
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
        public string SourcePath { get; private set; }

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
            OriginalMapTables result =
                Parse(bytes, levelIndex);
            result.SourcePath =
                Path.GetFullPath(path);
            return result;
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

        public bool TryGetWallClassVariant(
            byte rawWallId,
            byte expectedWallClass,
            out byte variant)
        {
            variant = 0;

            if (WallClass[rawWallId] != expectedWallClass)
                return false;

            for (int i = 0; i <= rawWallId; i++)
            {
                if (WallClass[i] == expectedWallClass)
                {
                    variant = (byte)(rawWallId - i);
                    return true;
                }
            }

            return false;
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
        /// Stable clean-room handle for the original OBJECT +0x0C map-cell binding.
        /// The Win16 value is a far pointer into the 64x64x2 MAP buffer; behavior
        /// only depends on cell identity, so the port stores a nonzero cell index.
        /// </summary>
        public static uint CellBindingHandle(int x, int y)
        {
            if (x < 0 || y < 0 ||
                x >= OriginalRuntime.MapWidth ||
                y >= OriginalRuntime.MapHeight)
            {
                return 0;
            }

            return (uint)(1 + y * OriginalRuntime.MapWidth + x);
        }

        /// <summary>
        /// Exact externally visible MAP-byte transfer from FUN_1010_7A44
        /// (GUARD state 0x13). If the candidate remains in the current cell,
        /// no occupancy test is performed. On a cell crossing the destination
        /// object byte must be zero and may not be the player's cell; the source
        /// object byte is then moved to the destination.
        /// </summary>
        public bool TryTransferState13ObjectCell(
            int sourceX,
            int sourceY,
            int targetX,
            int targetY,
            int playerX,
            int playerY)
        {
            if (sourceX < 0 || sourceY < 0 ||
                targetX < 0 || targetY < 0 ||
                sourceX >= OriginalRuntime.MapWidth ||
                sourceY >= OriginalRuntime.MapHeight ||
                targetX >= OriginalRuntime.MapWidth ||
                targetY >= OriginalRuntime.MapHeight)
            {
                return false;
            }

            if (sourceX == targetX && sourceY == targetY)
                return true;

            if (ObjectId[targetX, targetY] != 0)
                return false;

            if (targetX == playerX && targetY == playerY)
                return false;

            ObjectId[targetX, targetY] = ObjectId[sourceX, sourceY];
            ObjectId[sourceX, sourceY] = 0;
            return true;
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
            OriginalWallRuntime wallRuntime,
            out int doorTileX,
            out int doorTileY)
        {
            return TryFindNearestReachableDoor(
                startTileX,
                startTileY,
                (x, y) =>
                    wallRuntime != null &&
                    wallRuntime.DoorAllowsSight(x, y),
                out doorTileX,
                out doorTileY);
        }

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
        /// Primary-wall portion of player collision FUN_1010_84F4.
        /// Hard walls block immediately. Dynamic doors are passable only in
        /// controller states 0 or 4 (the shared FUN_1476 predicate).
        /// </summary>
        public bool IsPlayerPrimaryWallBlocked84F4(
            int x,
            int y,
            OriginalWallRuntime wallRuntime)
        {
            if (x < 0 || y < 0 ||
                x >= OriginalRuntime.MapWidth ||
                y >= OriginalRuntime.MapHeight)
            {
                return true;
            }

            byte flags = WallPropertyAt(x, y);

            if ((flags & WallHardBlock) != 0)
                return true;

            if ((flags & WallDynamicDoor) != 0)
            {
                return wallRuntime == null ||
                       !wallRuntime.DoorAllowsSight(x, y);
            }

            return false;
        }

        /// <summary>
        /// Final object-blocking test of player collision FUN_1010_84F4.
        /// Unlike D50A LOS, this path does NOT honor the object 0x20
        /// pass-through exception: object property bit 0x02 blocks directly.
        /// </summary>
        public bool IsPlayerObjectBlocked84F4(
            int x,
            int y)
        {
            if (x < 0 || y < 0 ||
                x >= OriginalRuntime.MapWidth ||
                y >= OriginalRuntime.MapHeight)
            {
                return true;
            }

            return (ObjectPropertyAt(x, y) &
                    ObjectBlocksMovementOrLos) != 0;
        }

        /// <summary>
        /// Cell-blocking portion of FUN_1010_D50A after Bresenham stepping,
        /// backed directly by the recovered 22-byte DOOR runtime state.
        /// </summary>
        public bool IsPerceptionIntermediateBlocked(
            int x,
            int y,
            bool secondaryCellChecks,
            OriginalWallRuntime wallRuntime)
        {
            return IsPerceptionIntermediateBlocked(
                x,
                y,
                secondaryCellChecks,
                (doorX, doorY) =>
                    wallRuntime != null &&
                    wallRuntime.DoorAllowsSight(doorX, doorY));
        }

        /// <summary>
        /// Compatibility overload for isolated tests/callers that provide only
        /// the dynamic-door passability predicate.
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
        public byte FindWallIdByClass(
            byte wallClass,
            byte startId)
        {
            for (int i = startId;
                i < 256;
                i++)
            {
                if (WallClass[i] == wallClass)
                {
                    return (byte)i;
                }
            }

            for (int i = 0;
                i < startId;
                i++)
            {
                if (WallClass[i] == wallClass)
                {
                    return (byte)i;
                }
            }

            throw new InvalidDataException(
                "No wall ID found for class 0x" +
                wallClass.ToString("X2"));
        }

        /// <summary>
        /// FUN_1018_3CFC.
        ///
        /// Odd classes 31,33,...,3F are visible on orientations 2/3.
        /// Even classes 32,34,...,40 are visible on orientations 0/1.
        /// </summary>
        public static bool ClassVisibleOnOrientation(
            int orientation,
            byte wallClass)
        {
            if (wallClass >= 0x31 &&
                wallClass <= 0x3F &&
                (wallClass & 1) != 0)
            {
                return orientation == 2 ||
                       orientation == 3;
            }

            if (wallClass >= 0x32 &&
                wallClass <= 0x40 &&
                (wallClass & 1) == 0)
            {
                return orientation == 0 ||
                       orientation == 1;
            }

            return false;
        }

        /// <summary>
        /// Semantic port of FUN_1018_4046 + 3D54 + 3E82 + 4006.
        ///
        /// The only non-original field is TextureIndex, supplied by the
        /// OpenNitemare3D bridge for the current cell.
        /// </summary>
        public List<OriginalRendererCore.Vec>
            BuildVectors(
                Func<int, int, int> textureIndexAt)
        {
            if (textureIndexAt == null)
            {
                throw new ArgumentNullException(
                    nameof(textureIndexAt));
            }

            List<OriginalRendererCore.Vec> vectors =
                new List<OriginalRendererCore.Vec>();

            for (int orientation = 0;
                orientation < 4;
                orientation++)
            {
                ScanOrientation(
                    orientation,
                    vectors,
                    textureIndexAt);
            }

            return vectors;
        }

        void ScanOrientation(
            int orientation,
            List<OriginalRendererCore.Vec> vectors,
            Func<int, int, int> textureIndexAt)
        {
            bool mergeActive = false;
            bool previousObjectClass3 = false;
            byte previousBit10 = 0;
            byte previousWallId = 0;
            byte previousNeighborWallId = 0;

            for (int outer = 0;
                outer < 64;
                outer++)
            {
                mergeActive = false;
                previousObjectClass3 = false;
                previousBit10 = 0;
                previousWallId = 0;
                previousNeighborWallId = 0;

                for (int inner = 0;
                    inner < 64;
                    inner++)
                {
                    if (vectors.Count >=
                        OriginalRendererCore.MaxVectors)
                    {
                        throw new InvalidOperationException(
                            "MAXVEC exceeded (1000).");
                    }

                    int x =
                        orientation == 0 ||
                        orientation == 1
                        ? inner
                        : outer;

                    int y =
                        orientation == 0 ||
                        orientation == 1
                        ? outer
                        : inner;

                    int dx =
                        orientation == 2
                        ? 1
                        : orientation == 3
                            ? -1
                            : 0;

                    int dy =
                        orientation == 0
                        ? -1
                        : orientation == 1
                            ? 1
                            : 0;

                    int nx = x + dx;
                    int ny = y + dy;

                    if (nx < 0 || nx >= 64 ||
                        ny < 0 || ny >= 64)
                    {
                        continue;
                    }

                    byte currentWall =
                        WallId[x, y];

                    byte neighborWall =
                        WallId[nx, ny];

                    byte currentProperty =
                        WallProperty[currentWall];

                    byte neighborProperty =
                        WallProperty[neighborWall];

                    byte currentBit10 =
                        (byte)(
                            (currentProperty & 0x10) >> 4);

                    bool dynamicNeighborBoundary =
                        (((currentProperty & 0x10) == 0) ||
                         neighborWall != currentWall) &&
                        ((neighborProperty & 0x10) != 0);

                    bool currentObjectClass3 =
                        ObjectClass[
                            ObjectId[x, y]] == 0x03;

                    bool neighborObjectClass3 =
                        ObjectClass[
                            ObjectId[nx, ny]] == 0x03;

                    bool class3Boundary =
                        neighborObjectClass3 &&
                        !(currentObjectClass3 &&
                          neighborWall == currentWall);

                    if ((currentProperty & 0x04) == 0)
                    {
                        byte c =
                            WallClass[currentWall];

                        if (!ClassVisibleOnOrientation(
                            orientation,
                            c))
                        {
                            mergeActive = false;
                        }
                        else
                        {
                            vectors.Add(
                                CreateHalfVec(
                                    x,
                                    y,
                                    currentWall,
                                    orientation,
                                    textureIndexAt(x, y)));

                            // The original does not explicitly change
                            // its merge-active byte on this successful
                            // half-vector path. Preserve that behavior.
                        }
                    }
                    else if (
                        (neighborProperty & 0x04) == 0 ||
                        class3Boundary ||
                        dynamicNeighborBoundary)
                    {
                        bool compatibleMerge =
                            mergeActive &&
                            previousWallId == currentWall &&
                            ((WallProperty[
                                previousNeighborWallId] ^
                              neighborProperty) & 0x08) == 0 &&
                            previousObjectClass3 ==
                                currentObjectClass3 &&
                            previousBit10 ==
                                currentBit10;

                        if (compatibleMerge)
                        {
                            ExtendVec(
                                vectors[
                                    vectors.Count - 1],
                                orientation);
                        }
                        else
                        {
                            vectors.Add(
                                CreateFullVec(
                                    x,
                                    y,
                                    currentWall,
                                    neighborWall,
                                    orientation,
                                    textureIndexAt(x, y)));

                            mergeActive = true;
                        }
                    }
                    else
                    {
                        mergeActive = false;
                    }

                    previousNeighborWallId =
                        neighborWall;

                    previousBit10 =
                        currentBit10;

                    previousObjectClass3 =
                        currentObjectClass3;

                    previousWallId =
                        currentWall;
                }
            }
        }

        OriginalRendererCore.Vec CreateFullVec(
            int x,
            int y,
            byte currentWall,
            byte neighborWall,
            int orientation,
            int textureIndex)
        {
            OriginalRendererCore.Vec vec =
                new OriginalRendererCore.Vec();

            vec.WallId = currentWall;
            vec.AnimationAux = 0;
            vec.AnimationFrame = 0;
            vec.RenderClass =
                WallClass[currentWall];
            vec.Flags =
                WallProperty[currentWall];
            vec.Orientation =
                (byte)orientation;
            vec.RuntimeTimer = 0;
            vec.SourceTileX = x;
            vec.SourceTileY = y;
            vec.TextureIndex = textureIndex;

            int xx = x;
            int yy = y;

            if (orientation == 2)
            {
                xx++;
            }

            vec.X1 =
                OriginalRendererCore.Wrap16(
                    xx << 6);

            if (orientation == 1)
            {
                yy++;
            }

            vec.Y1 =
                OriginalRendererCore.Wrap16(
                    yy << 6);

            vec.X2 =
                OriginalRendererCore.Wrap16(
                    vec.X1 +
                    ((orientation == 0 ||
                      orientation == 1)
                        ? 0x40
                        : 0));

            vec.Y2 =
                OriginalRendererCore.Wrap16(
                    vec.Y1 +
                    ((orientation == 2 ||
                      orientation == 3)
                        ? 0x40
                        : 0));

            byte firstId =
                FindWallIdByClass(
                    vec.RenderClass,
                    0);

            vec.TextureOffset =
                unchecked(
                    (sbyte)(
                        currentWall -
                        firstId));

            byte neighborClass =
                WallClass[neighborWall];

            bool neighborCurtainClass =
                neighborClass == 0x3F ||
                neighborClass == 0x40;

            if ((WallProperty[neighborWall] &
                    0x08) != 0)
            {
                bool neighborFacesThisEdge =
                    ClassVisibleOnOrientation(
                        orientation,
                        neighborClass);

                if (!neighborFacesThisEdge &&
                    !neighborCurtainClass)
                {
                    // Original changes only VEC +00 here. Flags/class
                    // stay those of the original current wall.
                    vec.WallId =
                        FindWallIdByClass(
                            0x30,
                            neighborWall);
                }
            }

            return vec;
        }

        OriginalRendererCore.Vec CreateHalfVec(
            int x,
            int y,
            byte wallId,
            int orientation,
            int textureIndex)
        {
            OriginalRendererCore.Vec vec =
                new OriginalRendererCore.Vec();

            vec.WallId = wallId;
            vec.AnimationAux = 0;
            vec.AnimationFrame = 0;
            vec.RenderClass =
                WallClass[wallId];
            vec.Flags =
                WallProperty[wallId];
            vec.Orientation =
                (byte)orientation;
            vec.RuntimeTimer = 0;
            vec.SourceTileX = x;
            vec.SourceTileY = y;
            vec.TextureIndex = textureIndex;

            int halfX =
                (orientation == 2 ||
                 orientation == 3)
                ? 0x20
                : 0;

            int halfY =
                (orientation == 0 ||
                 orientation == 1)
                ? 0x20
                : 0;

            vec.X1 =
                OriginalRendererCore.Wrap16(
                    x * 0x40 +
                    halfX);

            vec.Y1 =
                OriginalRendererCore.Wrap16(
                    y * 0x40 +
                    halfY);

            vec.X2 =
                OriginalRendererCore.Wrap16(
                    (x +
                     ((orientation == 0 ||
                       orientation == 1)
                        ? 1
                        : 0)) *
                    0x40 +
                    halfX);

            vec.Y2 =
                OriginalRendererCore.Wrap16(
                    (y +
                     ((orientation == 2 ||
                       orientation == 3)
                        ? 1
                        : 0)) *
                    0x40 +
                    halfY);

            if (vec.RenderClass == 0x3D ||
                vec.RenderClass == 0x3E)
            {
                byte first =
                    FindWallIdByClass(
                        0x3E,
                        0);

                vec.TextureOffset =
                    unchecked(
                        (sbyte)(
                            wallId - first));
            }
            else
            {
                byte first =
                    FindWallIdByClass(
                        vec.RenderClass,
                        0);

                int delta =
                    wallId - first;

                vec.TextureOffset =
                    unchecked(
                        (sbyte)(
                            delta / 2));
            }

            return vec;
        }

        static void ExtendVec(
            OriginalRendererCore.Vec vec,
            int orientation)
        {
            if (orientation == 0 ||
                orientation == 1)
            {
                vec.X2 =
                    OriginalRendererCore.Wrap16(
                        vec.X2 + 0x40);
            }

            if (orientation == 2 ||
                orientation == 3)
            {
                vec.Y2 =
                    OriginalRendererCore.Wrap16(
                        vec.Y2 + 0x40);
            }
        }

    }
}
