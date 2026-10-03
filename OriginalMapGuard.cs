namespace Nitemare3D
{
    /// <summary>
    /// Minimal entity/presentation shell for MAP actors whose behavior is owned
    /// entirely by the recovered 28-byte OBJECT + 26-byte GUARD runtime.
    /// It deliberately contains no legacy GuardState AI.
    /// </summary>
    public sealed class OriginalMapGuard :
        Entity,
        ISprite,
        IOriginalSpriteProjectionSource
    {
        readonly byte rawObjectId;
        bool runtimeBound;

        public int spriteIndex { get; set; }
        public Vec2 spritePosition { get; set; } = new Vec2();
        public bool visible { get; set; } = true;
        public float yOffset { get; set; }

        public OriginalMapGuard(byte rawObjectId)
        {
            this.rawObjectId = rawObjectId;
            Game.player.AddSprite(this);
            hasCollision = true;
        }

        public override void Start()
        {
            runtimeBound =
                OriginalRuntimeState.RegisterMapGuard(
                    this,
                    rawObjectId);

            SyncPresentation();
        }

        void SyncPresentation()
        {
            spritePosition = position;

            if (!runtimeBound ||
                !OriginalRuntimeState.TryGetObjectRecord(
                    this,
                    out var obj))
            {
                visible = false;
                hasCollision = false;
                return;
            }

            bool active =
                (obj.Flags &
                 OriginalMapTables.ObjectRuntimePresent) != 0;

            visible = active;
            hasCollision =
                active &&
                (obj.Flags &
                 OriginalMapTables.ObjectBlocksMovementOrLos) != 0;
            yOffset = obj.Runtime1A;
        }

        public override void Update()
        {
            if (runtimeBound &&
                OriginalRuntimeState.AutonomousGuardRuntimeEnabled &&
                OriginalRuntimeState.GuardLogicTickDue)
            {
                OriginalRuntimeState.TickConfirmedAutonomousState(
                    this);
            }

            SyncPresentation();
        }

        public bool TryGetOriginalSpriteFrame(
            out BitmapImage frame)
        {
            frame = null;

            if (!runtimeBound ||
                Img.current == null ||
                !OriginalRuntimeState.TryGetObjectRecord(
                    this,
                    out var obj))
            {
                return false;
            }

            return OriginalRuntimeState.ObjectDefinitions
                .TryGetBitmapFrame(
                    obj.DefinitionId,
                    unchecked((byte)obj.Component03),
                    Img.current.rawData,
                    out frame);
        }

        public bool TryGetOriginalProjectionObject(
            out OriginalObjectRecord runtimeObject)
        {
            return OriginalRuntimeState.TryGetObjectRecord(
                this,
                out runtimeObject);
        }

        public void RecordOriginalProjectedBaseRow(
            short projectedBaseRow)
        {
            OriginalRuntimeState.RecordObjectProjection(
                this,
                projectedBaseRow);
        }
    }
}
