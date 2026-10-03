using System;

namespace Nitemare3D
{
    /// <summary>
    /// Literal semantic C# port of Win16 NITE3W 1.10 FUN_1010_E798
    /// for the axis-aligned VEC records produced by the original map scan.
    ///
    /// Uses the exact Q10 trig table and recovered fixed-point constants.
    /// </summary>
    public static class OriginalProjectionExact
    {
        static readonly OriginalRendererCore.ProjectionConstants constants =
            OriginalRendererCore.BuildProjectionConstants(
                OriginalRendererCore.ViewportWidth,
                OriginalRendererCore.ViewportHeight);

        public static bool ProjectVec(
            OriginalRendererCore.Vec vec,
            short playerX,
            short playerY,
            int angleDegrees,
            OriginalTrigQ10 trig)
        {
            if (vec == null)
            {
                throw new ArgumentNullException(
                    nameof(vec));
            }

            if (trig == null)
            {
                throw new ArgumentNullException(
                    nameof(trig));
            }

            int angle =
                OriginalTrigQ10.Normalize(
                    angleDegrees);

            int cosQ10 =
                trig.Cos(angle);

            int sinQ10 =
                trig.Sin(angle);

            int dx1 =
                vec.X1 - playerX;

            int dx2 =
                vec.X2 - playerX;

            int dy1 =
                playerY - vec.Y1;

            int dy2 =
                playerY - vec.Y2;

            long rawDepth1 =
                (long)dx1 * cosQ10 +
                (long)dy1 * sinQ10;

            long rawDepth2 =
                (long)dx2 * cosQ10 +
                (long)dy2 * sinQ10;

            bool clipped1 =
                rawDepth1 <
                OriginalRendererCore.NearDepthQ10;

            bool clipped2 =
                rawDepth2 <
                OriginalRendererCore.NearDepthQ10;

            long depth1 =
                clipped1
                ? OriginalRendererCore.NearDepthQ10
                : rawDepth1;

            long depth2 =
                clipped2
                ? OriginalRendererCore.NearDepthQ10
                : rawDepth2;

            int projectedY1Q4 =
                (int)(
                    constants.VerticalNumerator /
                    depth1) +
                OriginalRendererCore.CenterYQ4;

            int projectedY2Q4 =
                (int)(
                    constants.VerticalNumerator /
                    depth2) +
                OriginalRendererCore.CenterYQ4;

            // E798: (viewportBottom + 0xB4) << 4.
            int verticalCullQ4 =
                (OriginalRendererCore.ViewBottom +
                 0xB4) << 4;

            if (projectedY1Q4 >
                    verticalCullQ4 &&
                projectedY2Q4 >
                    verticalCullQ4)
            {
                return false;
            }

            long adjustedDx1 = dx1;
            long adjustedDy1 = dy1;
            long adjustedDx2 = dx2;
            long adjustedDy2 = dy2;

            bool verticalWall =
                dx2 == dx1;

            if (clipped1)
            {
                MoveEndpointToNearPlane(
                    verticalWall,
                    angle,
                    cosQ10,
                    sinQ10,
                    ref adjustedDx1,
                    ref adjustedDy1);
            }

            if (clipped2)
            {
                MoveEndpointToNearPlane(
                    verticalWall,
                    angle,
                    cosQ10,
                    sinQ10,
                    ref adjustedDx2,
                    ref adjustedDy2);
            }

            long lateral1 =
                (long)sinQ10 * adjustedDx1 -
                (long)cosQ10 * adjustedDy1;

            long lateral2 =
                (long)sinQ10 * adjustedDx2 -
                (long)cosQ10 * adjustedDy2;

            long projectedX1 =
                lateral1 *
                constants.HorizontalScale /
                depth1 +
                OriginalRendererCore.CenterX;

            long projectedX2 =
                lateral2 *
                constants.HorizontalScale /
                depth2 +
                OriginalRendererCore.CenterX;

            short sx1 =
                ClampProjectedX(
                    projectedX1);

            short sx2 =
                ClampProjectedX(
                    projectedX2);

            short sy1 =
                OriginalRendererCore.Wrap16(
                    projectedY1Q4);

            short sy2 =
                OriginalRendererCore.Wrap16(
                    projectedY2Q4);

            if (sx2 < sx1)
            {
                short tempX = sx1;
                sx1 = sx2;
                sx2 = tempX;

                short tempY = sy1;
                sy1 = sy2;
                sy2 = tempY;
            }

            vec.ScreenX1 = sx1;
            vec.ProjectedY1Q4 = sy1;
            vec.ScreenX2 = sx2;
            vec.ProjectedY2Q4 = sy2;

            vec.CameraDepth1 = depth1;
            vec.CameraDepth2 = depth2;
            vec.CameraLateral1 = lateral1;
            vec.CameraLateral2 = lateral2;

            return
                sx1 <=
                    OriginalRendererCore.ViewRight &&
                sx2 >=
                    OriginalRendererCore.ViewLeft;
        }

        static void MoveEndpointToNearPlane(
            bool verticalWall,
            int angle,
            int cosQ10,
            int sinQ10,
            ref long dx,
            ref long dy)
        {
            long near =
                OriginalRendererCore.NearDepthQ10;

            if (verticalWall)
            {
                if (angle == 0x5A ||
                    angle == 0x10E)
                {
                    // Literal E798 cardinal special case.
                    dy = near;
                }
                else
                {
                    if (sinQ10 == 0)
                    {
                        throw new DivideByZeroException(
                            "E798 vertical-wall near-plane branch reached with zero sine.");
                    }

                    dy =
                        (near -
                         (long)cosQ10 * dx) /
                        sinQ10;
                }

                return;
            }

            if (angle == 0 ||
                angle == 0xB4)
            {
                // Literal E798 cardinal special case.
                dx = near;
            }
            else
            {
                if (cosQ10 == 0)
                {
                    throw new DivideByZeroException(
                        "E798 horizontal-wall near-plane branch reached with zero cosine.");
                }

                dx =
                    (near -
                     (long)sinQ10 * dy) /
                    cosQ10;
            }
        }

        static short ClampProjectedX(
            long value)
        {
            if (value < -0x3FFF)
            {
                return unchecked(
                    (short)0xC001);
            }

            if (value > 0x3FFF)
            {
                return 0x3FFF;
            }

            return unchecked(
                (short)value);
        }
    }
}