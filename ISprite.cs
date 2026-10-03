namespace Nitemare3D
{
    /// <summary>
    /// Optional bridge for sprites whose active visual frame is owned by the
    /// recovered IMG runtime rather than the historical flat spriteIndex list.
    /// </summary>
    public interface IOriginalSpriteFrameSource
    {
        bool TryGetOriginalSpriteFrame(out BitmapImage frame);
    }

    /// <summary>
    /// Optional extension for sprites backed by an original 28-byte OBJECT.
    /// It lets the renderer use the recovered CC7C projection and write the
    /// projection cache back to OBJECT+0x18.
    /// </summary>
    public interface IOriginalSpriteProjectionSource :
        IOriginalSpriteFrameSource
    {
        bool TryGetOriginalProjectionObject(
            out OriginalObjectRecord runtimeObject);

        void RecordOriginalProjectedBaseRow(
            short projectedBaseRow);
    }

    public interface ISprite
    {
        int spriteIndex{get;set;}
        Vec2 spritePosition{get;set;}
        bool visible{get;set;}
        float yOffset{get;set;}
    }
}