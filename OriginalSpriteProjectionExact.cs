using System;

namespace Nitemare3D
{
    /// <summary>
    /// Exact CC7C/B012 object-sprite rectangle construction layered on the
    /// recovered E5D8 point projection. Wall occlusion/queue ownership remains
    /// a caller-side concern.
    /// </summary>
    public static class OriginalSpriteProjectionExact
    {
        public struct ProjectedSprite
        {
            public int Left;
            public int Right;
            public int CenterX;
            public int Top;
            public int Bottom;
            public int BaselineRow;
            public ushort ProjectedYQ4;
            public int Width;
            public int Height;
            public long DepthQ10;
        }

        public static bool TryProject(
            OriginalObjectRecord obj,
            BitmapImage frame,
            short playerWorldX,
            short playerWorldY,
            int angleDegrees,
            OriginalTrigQ10 trig,
            out ProjectedSprite projected)
        {
            projected = default;

            if (frame == null ||
                frame.width == 0 ||
                frame.height == 0 ||
                (obj.Flags & 0x01) == 0)
            {
                return false;
            }

            int dx =
                obj.WorldX -
                playerWorldX;

            int dy =
                playerWorldY -
                obj.WorldY;

            if (!OriginalProjectionExact.ProjectPoint(
                    0,
                    dx,
                    dy,
                    angleDegrees,
                    trig,
                    out var point))
            {
                return false;
            }

            int baseline =
                point.ProjectedYQ4 >> 4;

            // CC7C vertical early-out: baseline beyond bottom+200.
            if (baseline >
                OriginalRendererCore.ViewBottom + 200)
            {
                return false;
            }

            int height =
                ((int)frame.height *
                 (baseline -
                  OriginalRendererCore.CenterY)) >> 5;

            if (height <= 0)
                return false;

            int bottom =
                baseline;

            sbyte verticalOffset =
                unchecked((sbyte)obj.Runtime1A);

            if (verticalOffset > 0)
            {
                bottom =
                    baseline -
                    (((verticalOffset *
                       (baseline -
                        OriginalRendererCore.CenterY)) >> 5) +
                     1);
            }

            int top =
                bottom -
                height;

            if (top >
                    OriginalRendererCore.ViewBottom ||
                OriginalRendererCore.ViewTop >
                    bottom)
            {
                return false;
            }

            int width =
                (((int)frame.width *
                  height +
                  frame.height) -
                 1) /
                frame.height;

            if (width <= 0)
                return false;

            int left =
                point.ScreenX -
                (width >> 1);

            int right =
                left +
                width;

            if (left >
                    OriginalRendererCore.ViewRight ||
                OriginalRendererCore.ViewLeft >
                    right)
            {
                return false;
            }

            projected = new ProjectedSprite
            {
                Left = left,
                Right = right,
                CenterX = point.ScreenX,
                Top = top,
                Bottom = bottom,
                BaselineRow = baseline,
                ProjectedYQ4 =
                    unchecked((ushort)point.ProjectedYQ4),
                Width = width,
                Height = height,
                DepthQ10 = point.DepthQ10
            };

            return true;
        }
    }
}
