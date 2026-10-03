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

    public interface ISprite
    {
        int spriteIndex{get;set;}
        Vec2 spritePosition{get;set;}
        bool visible{get;set;}
        float yOffset{get;set;}
    }
}