using System;
using System.Collections.Generic;

namespace Nitemare3D
{
    /// <summary>
    /// Runtime VEC mutation model reconstructed from Win16 NITE3W 1.10:
    ///
    /// - FUN_1010_14A8 paired-wall controller construction;
    /// - FUN_1010_188A paired-wall toggle/state propagation;
    /// - FUN_1010_1D4E auto-close decision;
    /// - FUN_1010_1E00 paired-wall and class-3 geometry movement;
    /// - FUN_1018_3736 / 37A0 VEC-to-map bounds lookup.
    ///
    /// The controller model mutates the same VEC objects consumed by the
    /// Stage-3 visibility renderer. This is the key difference from the old
    /// OpenNitemare3D Tile-only door animation.
    /// </summary>
    public sealed class OriginalWallRuntime
    {
        public sealed class PairedWall
        {
            public OriginalRendererCore.Vec First;
            public OriginalRendererCore.Vec Second;

            public int CellX;
            public int CellY;

            // Byte-exact 22-byte controller payload. The original +00/+04/+08
            // fields are far pointers; C# object references remain alongside the
            // record while all behavioral state lives in this exact layout.
            public OriginalDoorRuntimeRecord Runtime;

            public short State
            {
                get => Runtime.State;
                set => Runtime.State = value;
            }

            public short Timer
            {
                get => Runtime.Timer;
                set => Runtime.Timer = value;
            }

            public short TargetX
            {
                get => Runtime.TargetX;
                set => Runtime.TargetX = value;
            }

            public short TargetY
            {
                get => Runtime.TargetY;
                set => Runtime.TargetY = value;
            }

            public byte Latch
            {
                get => Runtime.Latch;
                set => Runtime.Latch = value;
            }
        }

        public sealed class Class3Group
        {
            public readonly OriginalRendererCore.Vec[] Component =
                new OriginalRendererCore.Vec[4];

            public int CellX;
            public int CellY;

            // Builder initializes +0x14 to 1.
            // Motion path acts only when the value is 2.
            public short Phase = 1;
        }

        readonly OriginalMapTables map;
        readonly List<OriginalRendererCore.Vec> vectors;

        readonly List<PairedWall> paired =
            new List<PairedWall>();

        readonly List<Class3Group> class3 =
            new List<Class3Group>();

        readonly Dictionary<int, PairedWall> pairedByCell =
            new Dictionary<int, PairedWall>();

        readonly Dictionary<int, Class3Group> class3ByCell =
            new Dictionary<int, Class3Group>();

        public IReadOnlyList<PairedWall> PairedWalls
        {
            get { return paired; }
        }

        public IReadOnlyList<Class3Group> Class3Groups
        {
            get { return class3; }
        }

        public OriginalWallRuntime(
            OriginalMapTables mapTables,
            List<OriginalRendererCore.Vec> vectorList)
        {
            map =
                mapTables ??
                throw new ArgumentNullException(
                    nameof(mapTables));

            vectors =
                vectorList ??
                throw new ArgumentNullException(
                    nameof(vectorList));

            BuildPairedWalls();
            BuildClass3Groups();
        }

        static int CellKey(
            int x,
            int y)
        {
            return
                (y << 6) |
                x;
        }

        void BuildPairedWalls()
        {
            paired.Clear();
            pairedByCell.Clear();

            // FUN_1010_14A8 scans Y outer / X inner.
            for (int y = 0;
                y < 64;
                y++)
            {
                for (int x = 0;
                    x < 64;
                    x++)
                {
                    byte wallId =
                        map.WallId[x, y];

                    if ((map.WallProperty[wallId] &
                            0x08) == 0)
                    {
                        continue;
                    }

                    if (paired.Count >= 64)
                    {
                        throw new InvalidOperationException(
                            "MAX paired-wall controller count exceeded (64).");
                    }

                    OriginalRendererCore.Vec first =
                        null;

                    OriginalRendererCore.Vec second =
                        null;

                    // Literal 14A8 selection:
                    // flags bit 08 and endpoint-1 cell equals scanned cell.
                    foreach (OriginalRendererCore.Vec vec
                        in vectors)
                    {
                        if ((vec.Flags & 0x08) == 0)
                        {
                            continue;
                        }

                        if ((vec.X1 >> 6) != x ||
                            (vec.Y1 >> 6) != y)
                        {
                            continue;
                        }

                        if (first == null)
                        {
                            first = vec;
                        }
                        else
                        {
                            second = vec;
                            break;
                        }
                    }

                    if (first == null ||
                        second == null)
                    {
                        throw new InvalidOperationException(
                            "Paired wall at " +
                            x + "," + y +
                            " does not have two matching VEC records.");
                    }

                    PairedWall controller =
                        new PairedWall();

                    controller.First = first;
                    controller.Second = second;
                    controller.CellX = x;
                    controller.CellY = y;
                    controller.State = 1;
                    controller.Timer = 0;
                    controller.Latch = 0;

                    bool flip =
                        false;

                    if (first.Orientation == 0 &&
                        x + 1 < 64)
                    {
                        byte neighbor =
                            map.WallId[
                                x + 1,
                                y];

                        flip =
                            (map.WallProperty[neighbor] &
                             0x08) != 0;
                    }
                    else if (
                        first.Orientation == 2 &&
                        y + 1 < 64)
                    {
                        byte neighbor =
                            map.WallId[
                                x,
                                y + 1];

                        flip =
                            (map.WallProperty[neighbor] &
                             0x08) != 0;
                    }

                    if (flip)
                    {
                        first.Flags |= 0x20;
                        second.Flags |= 0x20;

                        controller.TargetX =
                            second.X2;

                        controller.TargetY =
                            second.Y2;
                    }
                    else
                    {
                        first.Flags &=
                            unchecked((byte)~0x20);

                        second.Flags &=
                            unchecked((byte)~0x20);

                        controller.TargetX =
                            second.X1;

                        controller.TargetY =
                            second.Y1;
                    }

                    paired.Add(
                        controller);

                    pairedByCell[
                        CellKey(x, y)] =
                        controller;
                }
            }
        }

        void BuildClass3Groups()
        {
            class3.Clear();
            class3ByCell.Clear();

            for (int y = 0;
                y < 64;
                y++)
            {
                for (int x = 0;
                    x < 64;
                    x++)
                {
                    byte objectId =
                        map.ObjectId[x, y];

                    if (map.ObjectClass[objectId] != 0x03)
                    {
                        continue;
                    }

                    if (class3.Count >= 32)
                    {
                        throw new InvalidOperationException(
                            "MAX class-3 wall-group count exceeded (32).");
                    }

                    byte wallClass =
                        map.WallClass[
                            map.WallId[x, y]];

                    Class3Group group =
                        new Class3Group();

                    group.CellX = x;
                    group.CellY = y;
                    group.Phase = 1;

                    // FUN_1018_37A0 enumerates VECs in table order whose
                    // render class matches and whose 3736 bounds contain
                    // this cell.
                    foreach (OriginalRendererCore.Vec vec
                        in vectors)
                    {
                        if (vec.RenderClass != wallClass)
                        {
                            continue;
                        }

                        GetVecBounds(
                            vec,
                            out int minX,
                            out int minY,
                            out int maxX,
                            out int maxY);

                        if (x < minX ||
                            x > maxX ||
                            y < minY ||
                            y > maxY)
                        {
                            continue;
                        }

                        group.Component[
                            vec.Orientation] =
                            vec;
                    }

                    class3.Add(
                        group);

                    class3ByCell[
                        CellKey(x, y)] =
                        group;
                }
            }
        }

        public static void GetVecBounds(
            OriginalRendererCore.Vec vec,
            out int minX,
            out int minY,
            out int maxX,
            out int maxY)
        {
            // Literal FUN_1018_3736.
            minX =
                vec.X1 >> 6;

            maxX =
                vec.X2 >> 6;

            minY =
                vec.Y1 >> 6;

            maxY =
                vec.Y2 >> 6;

            if ((vec.Flags & 0x08) == 0)
            {
                if (vec.Orientation == 2)
                {
                    minX--;
                    maxX = minX;
                }

                if (vec.Orientation == 1)
                {
                    minY--;
                    maxY = minY;
                }
            }
        }

        public PairedWall FindPairedWall(
            int x,
            int y)
        {
            pairedByCell.TryGetValue(
                CellKey(x, y),
                out PairedWall controller);

            return controller;
        }

        public bool TryGetDoorRuntimeRecord(
            int x,
            int y,
            out OriginalDoorRuntimeRecord record)
        {
            PairedWall controller = FindPairedWall(x, y);
            if (controller == null)
            {
                record = default;
                return false;
            }

            record = controller.Runtime;
            return true;
        }

        public bool DoorAllowsSight(
            int x,
            int y)
        {
            PairedWall controller = FindPairedWall(x, y);
            return controller != null &&
                   OriginalGuardDispatcher.GuardLosRuntimeRecordPassable(
                       unchecked((ushort)controller.Runtime.State));
        }

        public Class3Group FindClass3Group(
            int x,
            int y)
        {
            class3ByCell.TryGetValue(
                CellKey(x, y),
                out Class3Group group);

            return group;
        }

        /// <summary>
        /// FUN_1010_188A state toggle without audio.
        ///
        /// State 0/2 -> 3 (closing)
        /// State 1/3 -> 2 (opening), subject to the 3D/3E directional gate.
        /// State 4 -> unchanged.
        ///
        /// playerSector is the same 0..7 direction-sector input consumed by
        /// the original class 3D/3E gate.
        /// </summary>
        public bool TogglePairedWall(
            int x,
            int y,
            int playerSector,
            bool setLatch = true)
        {
            PairedWall controller =
                FindPairedWall(
                    x,
                    y);

            if (controller == null ||
                controller.State == 4)
            {
                return false;
            }

            if (setLatch)
            {
                controller.Latch = 1;
            }

            short old =
                controller.State;

            if (old == 0 ||
                old == 2)
            {
                controller.State = 3;

                // Explicit close marks both halves collision-active
                // immediately.
                controller.First.Flags |= 0x01;
                controller.Second.Flags |= 0x01;

                // Close path clears one-shot sound latch.
                controller.Latch = 0;
            }
            else if (
                old == 1 ||
                old == 3)
            {
                byte wallClass =
                    controller.First.RenderClass;

                bool directionalAllowed =
                    true;

                if (wallClass == 0x3D ||
                    wallClass == 0x3E)
                {
                    int expected =
                        ((playerSector + 1) & 6) >> 1;

                    directionalAllowed =
                        expected ==
                        unchecked(
                            (byte)controller.First.TextureOffset);
                }

                if (directionalAllowed)
                {
                    controller.State = 2;
                }
            }

            PropagateState(
                controller);

            return
                controller.State != old;
        }

        public bool ForceState(
            int x,
            int y,
            short state)
        {
            PairedWall controller =
                FindPairedWall(
                    x,
                    y);

            if (controller == null ||
                controller.State == 4)
            {
                return false;
            }

            controller.State =
                state;

            if (state == 3)
            {
                controller.First.Flags |= 0x01;
                controller.Second.Flags |= 0x01;
            }

            PropagateState(
                controller);

            return true;
        }

        public bool SetLatchedPassable(
            int x,
            int y)
        {
            PairedWall controller =
                FindPairedWall(
                    x,
                    y);

            if (controller == null)
            {
                return false;
            }

            controller.State = 4;
            return true;
        }

        void PropagateState(
            PairedWall controller)
        {
            // 188A checks two neighboring controller cells along an axis
            // chosen from the first VEC orientation.
            int dx =
                controller.First.Orientation == 2
                ? 0
                : 1;

            int dy =
                controller.First.Orientation == 2
                ? 1
                : 0;

            PropagateOne(
                controller,
                controller.CellX + dx,
                controller.CellY + dy);

            PropagateOne(
                controller,
                controller.CellX - dx,
                controller.CellY - dy);
        }

        void PropagateOne(
            PairedWall source,
            int x,
            int y)
        {
            if (x < 0 || x >= 64 ||
                y < 0 || y >= 64)
            {
                return;
            }

            byte wallId =
                map.WallId[x, y];

            if ((map.WallProperty[wallId] &
                    0x08) == 0)
            {
                return;
            }

            PairedWall neighbor =
                FindPairedWall(
                    x,
                    y);

            if (neighbor == null ||
                Object.ReferenceEquals(
                    neighbor,
                    source))
            {
                return;
            }

            neighbor.State =
                source.State;

            if (source.State == 3)
            {
                neighbor.First.Flags |= 0x01;
                neighbor.Second.Flags |= 0x01;
            }
        }

        /// <summary>
        /// FUN_1010_1D4E auto-close decision.
        ///
        /// objectOccupied is the runtime map-cell OBJECT-byte test. The
        /// OpenNitemare3D bridge must supply current actor occupancy; using
        /// the static MAP object ID would be incorrect.
        /// </summary>
        public void TickAutoClose(
            int playerTileX,
            int playerTileY,
            Func<int, int, bool> objectOccupied)
        {
            foreach (PairedWall controller
                in paired)
            {
                if (controller.State != 0)
                {
                    continue;
                }

                byte wallClass =
                    controller.First.RenderClass;

                if (wallClass == 0x3B ||
                    wallClass == 0x3C)
                {
                    continue;
                }

                controller.Timer--;

                if (controller.Timer != 0)
                {
                    continue;
                }

                bool occupiedByObject =
                    objectOccupied != null &&
                    objectOccupied(
                        controller.CellX,
                        controller.CellY);

                bool playerHere =
                    playerTileX ==
                        controller.CellX &&
                    playerTileY ==
                        controller.CellY;

                if (!occupiedByObject &&
                    !playerHere)
                {
                    controller.Latch = 0;
                    controller.State = 3;

                    controller.First.Flags |= 0x01;
                    controller.Second.Flags |= 0x01;
                }
                else
                {
                    controller.Timer = 4;
                }
            }
        }

        /// <summary>
        /// Paired-wall part of FUN_1010_1E00.
        /// Call once per original-style frame update, AFTER rendering the
        /// current geometry, so the new endpoints appear on the next frame.
        /// </summary>
        public void TickPairedWallMotion()
        {
            foreach (PairedWall controller
                in paired)
            {
                if (controller.State != 2 &&
                    controller.State != 3)
                {
                    continue;
                }

                OriginalRendererCore.Vec first =
                    controller.First;

                OriginalRendererCore.Vec second =
                    controller.Second;

                bool flipped =
                    (first.Flags & 0x20) != 0;

                bool xAxis =
                    first.Orientation == 0;

                short current =
                    xAxis
                    ? (flipped
                        ? first.X2
                        : first.X1)
                    : (flipped
                        ? first.Y2
                        : first.Y1);

                short target =
                    xAxis
                    ? controller.TargetX
                    : controller.TargetY;

                if (controller.State == 2)
                {
                    target =
                        xAxis
                        ? (flipped
                            ? first.X1
                            : first.X2)
                        : (flipped
                            ? first.Y1
                            : first.Y2);
                }

                int delta =
                    controller.State == 2
                    ? 2
                    : -2;

                if (flipped)
                {
                    delta = -delta;
                }

                if (current != target)
                {
                    AddSelectedCoordinate(
                        first,
                        xAxis,
                        flipped,
                        delta);

                    AddSelectedCoordinate(
                        second,
                        xAxis,
                        flipped,
                        delta);
                }

                current =
                    xAxis
                    ? (flipped
                        ? first.X2
                        : first.X1)
                    : (flipped
                        ? first.Y2
                        : first.Y1);

                if (current == target)
                {
                    controller.State =
                        (short)(
                            controller.State == 2
                            ? 0
                            : 1);

                    controller.Timer = 0x20;

                    if (controller.State == 0)
                    {
                        first.Flags &=
                            unchecked((byte)~0x01);

                        second.Flags &=
                            unchecked((byte)~0x01);
                    }
                }
            }
        }

        static void AddSelectedCoordinate(
            OriginalRendererCore.Vec vec,
            bool xAxis,
            bool flipped,
            int delta)
        {
            if (xAxis)
            {
                if (flipped)
                {
                    vec.X2 =
                        OriginalRendererCore.Wrap16(
                            vec.X2 + delta);
                }
                else
                {
                    vec.X1 =
                        OriginalRendererCore.Wrap16(
                            vec.X1 + delta);
                }
            }
            else
            {
                if (flipped)
                {
                    vec.Y2 =
                        OriginalRendererCore.Wrap16(
                            vec.Y2 + delta);
                }
                else
                {
                    vec.Y1 =
                        OriginalRendererCore.Wrap16(
                            vec.Y1 + delta);
                }
            }
        }

        public bool ActivateClass3(
            int x,
            int y)
        {
            Class3Group group =
                FindClass3Group(
                    x,
                    y);

            if (group == null)
            {
                return false;
            }

            group.Phase = 2;
            return true;
        }

        /// <summary>
        /// Class-3 four-way portion of FUN_1010_1E00.
        /// </summary>
        public void TickClass3Motion()
        {
            foreach (Class3Group group
                in class3)
            {
                if (group.Phase != 2)
                {
                    continue;
                }

                group.Phase = 0;

                MoveComponentX(
                    group.Component[0],
                    ref group.Phase);

                MoveComponentY(
                    group.Component[1],
                    ref group.Phase);

                MoveComponentX(
                    group.Component[2],
                    ref group.Phase);

                MoveComponentY(
                    group.Component[3],
                    ref group.Phase);

                if (group.Phase == 0)
                {
                    CompleteSharedClass3Groups(
                        group);
                }
            }
        }

        static void MoveComponentX(
            OriginalRendererCore.Vec vec,
            ref short phase)
        {
            if (vec == null)
            {
                return;
            }

            if (vec.X1 < vec.X2)
            {
                vec.X1 =
                    OriginalRendererCore.Wrap16(
                        vec.X1 + 2);

                phase = 2;
            }
        }

        static void MoveComponentY(
            OriginalRendererCore.Vec vec,
            ref short phase)
        {
            if (vec == null)
            {
                return;
            }

            if (vec.Y1 < vec.Y2)
            {
                vec.Y1 =
                    OriginalRendererCore.Wrap16(
                        vec.Y1 + 2);

                phase = 2;
            }
        }

        void CompleteSharedClass3Groups(
            Class3Group completed)
        {
            foreach (Class3Group group
                in class3)
            {
                if (!SharesAnyComponent(
                    completed,
                    group))
                {
                    continue;
                }

                foreach (OriginalRendererCore.Vec vec
                    in group.Component)
                {
                    if (vec != null)
                    {
                        vec.Flags &=
                            unchecked((byte)~0x01);
                    }
                }

                map.WallId[
                    group.CellX,
                    group.CellY] = 0;

                map.ObjectId[
                    group.CellX,
                    group.CellY] = 0;

                group.Phase = 0;
            }
        }

        static bool SharesAnyComponent(
            Class3Group a,
            Class3Group b)
        {
            for (int i = 0;
                i < 4;
                i++)
            {
                if (a.Component[i] == null)
                {
                    continue;
                }

                for (int j = 0;
                    j < 4;
                    j++)
                {
                    if (Object.ReferenceEquals(
                        a.Component[i],
                        b.Component[j]))
                    {
                        return true;
                    }
                }
            }

            return false;
        }
    }
}