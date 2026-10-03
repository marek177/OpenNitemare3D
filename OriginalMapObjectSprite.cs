namespace Nitemare3D
{
    /// <summary>
    /// Generic presentation/runtime shell for MAP objects that have a recovered
    /// 28-byte OBJECT record but do not yet need a dedicated gameplay class.
    /// GUARD-linked classes are deliberately excluded by Level.SpawnMapObject.
    /// </summary>
    public sealed class OriginalMapObjectSprite :
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

        public OriginalMapObjectSprite(byte rawObjectId)
        {
            this.rawObjectId = rawObjectId;
            Game.player.AddSprite(this);
            hasCollision = false;
        }

        public override void Start()
        {
            runtimeBound =
                OriginalRuntimeState.RegisterWorldObject(
                    this,
                    rawObjectId);

            if (runtimeBound &&
                OriginalRuntimeState.TryGetObjectRecord(
                    this,
                    out var obj))
            {
                hasCollision =
                    (obj.Flags &
                     OriginalMapTables.ObjectBlocksMovementOrLos) != 0;
                yOffset = obj.Runtime1A;
            }
        }

        public override void Update()
        {
            spritePosition = position;

            if (runtimeBound &&
                OriginalRuntimeState.TryGetObjectRecord(
                    this,
                    out var obj))
            {
                yOffset = obj.Runtime1A;
                visible =
                    (obj.Flags &
                     OriginalMapTables.ObjectRuntimePresent) != 0;
            }
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

            int frameIndex =
                unchecked((byte)obj.Component03);

            return OriginalRuntimeState.ObjectDefinitions
                .TryGetBitmapFrame(
                    obj.DefinitionId,
                    frameIndex,
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
