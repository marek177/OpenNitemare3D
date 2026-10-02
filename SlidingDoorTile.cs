using System;

namespace Nitemare3D
{
    public enum DoorRuntimeState : byte
    {
        Open = 0,
        Closed = 1,
        Opening = 2,
        Closing = 3
    }

    // Runtime model recovered from the original 22-byte paired-wall/door controller.
    // The original moves one VEC endpoint by 2 world units per presentation update.
    public sealed class SlidingDoorTile : Tile
    {
        const int TravelUnits = 32;
        const int MotionStepUnits = 2;
        const int AutoCloseTicks = 32;
        const int BlockedRetryTicks = 4;

        // Original fast presentation path is calibrated from render time and clamps
        // to a 40 ms minimum render period, so its maximum update frequency is 25 Hz.
        const double PresentationStepSeconds = 1.0 / 25.0;

        // The delayed door timer is processed in the same 8 Hz slow bundle as the
        // recovered Magic Eye / Crystal Ball power logic.
        const double SlowStepSeconds = 1.0 / 8.0;

        double presentationAccumulator;
        double slowAccumulator;

        int openUnits;
        int autoCloseTimer = AutoCloseTicks;
        bool soundLatch;

        public byte WallClass { get; private set; }
        public DoorRuntimeState RuntimeState { get; private set; } = DoorRuntimeState.Closed;

        public float OpenFraction => openUnits / (float)TravelUnits;
        public bool IsFullyOpen => RuntimeState == DoorRuntimeState.Open;
        public bool IsMoving =>
            RuntimeState == DoorRuntimeState.Opening ||
            RuntimeState == DoorRuntimeState.Closing;

        public Automap.VecOrientation FacingOrientation(float rayDirX, float rayDirY)
        {
            if ((WallClass & 1) != 0)
            {
                return rayDirX > 0
                    ? Automap.VecOrientation.Left
                    : Automap.VecOrientation.Right;
            }

            return rayDirY > 0
                ? Automap.VecOrientation.Top
                : Automap.VecOrientation.Bottom;
        }

        public bool TryIntersectRay(
            Vec2 origin,
            float rayDirX,
            float rayDirY,
            out float distance,
            out float textureCoord)
        {
            distance = 0;
            textureCoord = 0;

            if (IsFullyOpen)
                return false;

            bool vertical = (WallClass & 1) != 0;
            Automap.VecOrientation orientation;

            if (vertical)
            {
                if (Math.Abs(rayDirX) < 0.000001f)
                    return false;

                float planeX = x + 0.5f;
                distance = (planeX - origin.X) / rayDirX;
                if (distance <= 0)
                    return false;

                float hitY = origin.Y + distance * rayDirY;
                textureCoord = (hitY - y) * 64.0f;
                orientation = FacingOrientation(rayDirX, rayDirY);
            }
            else
            {
                if (Math.Abs(rayDirY) < 0.000001f)
                    return false;

                float planeY = y + 0.5f;
                distance = (planeY - origin.Y) / rayDirY;
                if (distance <= 0)
                    return false;

                float hitX = origin.X + distance * rayDirX;
                textureCoord = (hitX - x) * 64.0f;
                orientation = FacingOrientation(rayDirX, rayDirY);
            }

            if (textureCoord < 0.0f || textureCoord > 64.0f)
                return false;

            float min = 0.0f;
            float max = 64.0f;

            // FUN_1010_1E00 shortens one endpoint by two units per update:
            // orientation 0/2 retract their far endpoint, orientation 1/3
            // advance their near endpoint.
            if (orientation == Automap.VecOrientation.Top ||
                orientation == Automap.VecOrientation.Right)
            {
                max -= openUnits;
            }
            else
            {
                min += openUnits;
            }

            return textureCoord >= min && textureCoord <= max;
        }

        public Automap.VecOrientation AutomapOrientation =>
            (WallClass & 1) != 0
                ? Automap.VecOrientation.Right
                : Automap.VecOrientation.Top;

        public bool AutomapFlag20
        {
            get
            {
                int nx = x;
                int ny = y;

                if ((WallClass & 1) != 0)
                    ny++;
                else
                    nx++;

                if (nx < 0 || ny < 0 || nx >= 64 || ny >= 64)
                    return false;

                byte otherClass = Level.GetWallClass(Level.tilemap[nx, ny]);
                return otherClass >= 0x31 && otherClass <= 0x40;
            }
        }

        public SlidingDoorTile(byte wallClass)
        {
            WallClass = wallClass;
            door = true;
            thin = true;
            obstacle = true;
        }

        public override void Create()
        {
            door = true;
            thin = true;
            obstacle = true;
            RuntimeState = DoorRuntimeState.Closed;
            openUnits = 0;
            autoCloseTimer = AutoCloseTicks;
            presentationAccumulator = 0;
            slowAccumulator = 0;
            soundLatch = false;
        }

        public bool IsRemoteControlled =>
            WallClass == RecoveredInteractionFacts.RemoteDoorVerticalClass ||
            WallClass == RecoveredInteractionFacts.RemoteDoorHorizontalClass;

        public bool RequiresKey =>
            WallClass >= 0x33 && WallClass <= 0x38;

        public bool RequiresIdCard =>
            WallClass >= 0x39 && WallClass <= 0x3A;

        public bool IsCurtain =>
            WallClass == 0x3F || WallClass == 0x40;

        public int CredentialGroup => Level.GetWallClassVariant(this);

        public override void OnUse()
        {
            // Remote doors are command-driven and reject ordinary USE.
            if (IsRemoteControlled)
                return;

            // Retail maps do not use 0x3D/0x3E. Their original common helper has
            // an extra direction-selector condition, so keep them gated until that
            // rare path is implemented rather than guessing.
            if (WallClass == 0x3D || WallClass == 0x3E)
                return;

            if (RequiresKey &&
                (Game.player == null || !Game.player.HasKeyGroup(CredentialGroup)))
            {
                return;
            }

            if (RequiresIdCard &&
                (Game.player == null || !Game.player.HasIdCardGroup(CredentialGroup)))
            {
                return;
            }

            ActivateCommon(true);
        }

        // Call after the recovered key/card gate accepts the door.
        public void ActivateAuthorized()
        {
            ActivateCommon(true);
        }

        // Remote command 0x1E: only closed/closing records are selected.
        public void RemoteOpen()
        {
            if (RuntimeState == DoorRuntimeState.Closed ||
                RuntimeState == DoorRuntimeState.Closing)
            {
                soundLatch = true;
                BeginOpening();
                PropagateStateToAdjacentDoors();
            }
        }

        // Remote command 0x1F: only open/opening records are selected.
        public void RemoteClose()
        {
            if (RuntimeState == DoorRuntimeState.Open ||
                RuntimeState == DoorRuntimeState.Opening)
            {
                soundLatch = true;
                BeginClosing();
                PropagateStateToAdjacentDoors();
            }
        }

        void ActivateCommon(bool withSound)
        {
            if (withSound)
                soundLatch = true;

            // Exact FUN_1010_188A state transitions:
            // 0/2 -> 3, 1/3 -> 2.
            if (RuntimeState == DoorRuntimeState.Open ||
                RuntimeState == DoorRuntimeState.Opening)
            {
                BeginClosing();
            }
            else
            {
                BeginOpening();
            }

            PropagateStateToAdjacentDoors();
        }

        void ApplyPropagatedState(DoorRuntimeState state)
        {
            RuntimeState = state;

            // State 2 and 3 are both non-passable in the original helper.
            obstacle = state != DoorRuntimeState.Open;
        }

        void PropagateStateToAdjacentDoors()
        {
            // FUN_1010_188A checks the two neighbouring MAP cells along the door axis.
            int dx = (WallClass & 1) == 0 ? 1 : 0;
            int dy = (WallClass & 1) != 0 ? 1 : 0;

            SyncNeighbour(x + dx, y + dy);
            SyncNeighbour(x - dx, y - dy);
        }

        void SyncNeighbour(int nx, int ny)
        {
            if (nx < 0 || ny < 0 || nx >= 64 || ny >= 64)
                return;

            if (Level.tilemap[nx, ny] is SlidingDoorTile neighbour)
                neighbour.ApplyPropagatedState(RuntimeState);
        }

        void BeginOpening()
        {
            RuntimeState = DoorRuntimeState.Opening;

            if (soundLatch)
                SoundEffect.PlaySound(SoundConsts.DOOR_OPEN);

            // Collision stays active through the complete opening motion.
            obstacle = true;
        }

        void BeginClosing()
        {
            RuntimeState = DoorRuntimeState.Closing;

            if (soundLatch)
                SoundEffect.PlaySound(SoundConsts.DOOR_CLOSE);

            soundLatch = false;

            // Original sets the VEC collision bit immediately when closing starts.
            obstacle = true;
        }

        bool Occupied()
        {
            // Original auto-close first tests MAP-cell object byte +1.
            if (Level.tilemap[x, y].objectID != 0)
                return true;

            if (Game.player != null &&
                (int)Game.player.position.X == x &&
                (int)Game.player.position.Y == y)
            {
                return true;
            }

            foreach (Entity entity in Entity.entities)
            {
                if (entity == Game.player)
                    continue;

                if ((int)entity.position.X == x &&
                    (int)entity.position.Y == y)
                {
                    return true;
                }
            }

            return false;
        }

        void MotionTick()
        {
            if (RuntimeState == DoorRuntimeState.Opening)
            {
                openUnits += MotionStepUnits;
                if (openUnits >= TravelUnits)
                {
                    openUnits = TravelUnits;
                    RuntimeState = DoorRuntimeState.Open;
                    autoCloseTimer = AutoCloseTicks;
                    obstacle = false;

                    Automap.DoorOpened(
                        x,
                        y,
                        AutomapOrientation,
                        AutomapFlag20);
                }
            }
            else if (RuntimeState == DoorRuntimeState.Closing)
            {
                openUnits -= MotionStepUnits;
                if (openUnits <= 0)
                {
                    openUnits = 0;
                    RuntimeState = DoorRuntimeState.Closed;
                    autoCloseTimer = AutoCloseTicks;
                    obstacle = true;
                    Automap.DoorClosed(x, y);
                }
            }
        }

        void SlowTick()
        {
            if (RuntimeState != DoorRuntimeState.Open || IsRemoteControlled)
                return;

            autoCloseTimer--;
            if (autoCloseTimer > 0)
                return;

            // Original checks player MAP cell and door-cell occupancy.
            // When blocked it retries four slow ticks later.
            if (Occupied())
            {
                autoCloseTimer = BlockedRetryTicks;
                return;
            }

            BeginClosing();
        }

        public override void Update()
        {
            // Original outer scheduler runs the 8 Hz simulation bundle before the
            // presentation branch that advances door geometry.
            slowAccumulator += Time.dt;
            while (slowAccumulator >= SlowStepSeconds)
            {
                slowAccumulator -= SlowStepSeconds;
                SlowTick();
            }

            presentationAccumulator += Time.dt;
            while (presentationAccumulator >= PresentationStepSeconds)
            {
                presentationAccumulator -= PresentationStepSeconds;
                MotionTick();
            }
        }
    }
}
