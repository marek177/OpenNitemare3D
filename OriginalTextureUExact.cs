using System;

namespace Nitemare3D
{
    /// <summary>
    /// Literal semantic port of Win16 FUN_1010_EBD6 together with the
    /// immediately surrounding 66B0 pre/post arithmetic that feeds 6422.
    ///
    /// The function returns the original 16-bit along-wall coordinate before
    /// the final width mask performed by SelectTextureU / FUN_1010_6422.
    /// </summary>
    public static class OriginalTextureUExact
    {
        static readonly OriginalRendererCore.ProjectionConstants projection =
            OriginalRendererCore.BuildProjectionConstants(
                OriginalRendererCore.ViewportWidth,
                OriginalRendererCore.ViewportHeight);

        /// <summary>
        /// DAT_1048_4BF0 is nonzero for octants selected by mask 0x99:
        /// 0,3,4,7.
        /// </summary>
        public static bool UsesCosineDivisor(
            int angleDegrees)
        {
            int octant =
                OriginalTrigQ10.Normalize(
                    angleDegrees) / 45;

            return
                (0x99 & (1 << octant)) != 0;
        }

        /// <summary>
        /// FUN_1010_EBD6 exactly at semantic integer-arithmetic level.
        ///
        /// orientation      VEC +07
        /// screenX         current column
        /// spanYQ4         high 16-bit word of span accumulator before step
        /// perpendicularProduct
        ///                 32-bit iVar9 * opposite trig coefficient from 66B0
        /// </summary>
        public static short PerspectiveCoordinate(
            int orientation,
            short screenX,
            ushort spanYQ4,
            int perpendicularProduct,
            int angleDegrees,
            OriginalTrigQ10 trig)
        {
            if (trig == null)
            {
                throw new ArgumentNullException(
                    nameof(trig));
            }

            if (spanYQ4 == 0)
            {
                throw new DivideByZeroException(
                    "Original EBD6 would divide by zero for spanYQ4 == 0.");
            }

            int angle =
                OriginalTrigQ10.Normalize(
                    angleDegrees);

            int sinQ10 =
                trig.Sin(angle);

            int cosQ10 =
                trig.Cos(angle);

            bool cosineDivisor =
                UsesCosineDivisor(angle);

            int divisor =
                cosineDivisor
                ? cosQ10
                : sinQ10;

            if (divisor == 0)
            {
                throw new DivideByZeroException(
                    "Original EBD6 selected a zero trig divisor.");
            }

            bool verticalOrientation =
                orientation == 2 ||
                orientation == 3;

            long temp;

            if (!cosineDivisor)
            {
                // DAT_4BF0 == 0, iVar1 = sin.
                if (verticalOrientation)
                {
                    long screenTerm =
                        ((long)(
                            screenX -
                            OriginalRendererCore.CenterX) *
                         projection.InverseProjectionScale) /
                        spanYQ4;

                    temp =
                        (long)perpendicularProduct -
                        screenTerm;
                }
                else
                {
                    temp =
                        projection.VerticalNumerator /
                        spanYQ4 -
                        perpendicularProduct;
                }
            }
            else
            {
                // DAT_4BF0 != 0, iVar1 = cos.
                if (verticalOrientation)
                {
                    temp =
                        projection.VerticalNumerator /
                        spanYQ4 -
                        perpendicularProduct;
                }
                else
                {
                    long screenTerm =
                        ((long)(
                            screenX -
                            OriginalRendererCore.CenterX) *
                         projection.InverseProjectionScale) /
                        spanYQ4;

                    temp =
                        screenTerm +
                        perpendicularProduct;
                }
            }

            long quotient =
                temp / divisor;

            return
                OriginalRendererCore.Wrap16(
                    (int)quotient);
        }

        /// <summary>
        /// Reconstructs the complete 66B0 -> EBD6 -> local_20/sign -> 6422
        /// texture-U coordinate for one wall column.
        /// </summary>
        public static ushort ComputeTextureU(
            OriginalRendererCore.Vec vec,
            short screenX,
            ushort spanYQ4,
            short playerX,
            short playerY,
            int angleDegrees,
            OriginalTrigQ10 trig,
            ushort textureWidth)
        {
            if (vec == null)
            {
                throw new ArgumentNullException(
                    nameof(vec));
            }

            if (textureWidth == 0)
            {
                throw new ArgumentOutOfRangeException(
                    nameof(textureWidth));
            }

            bool endpointFlip =
                (vec.Flags & 0x20) != 0;

            if (vec.RenderClass == 0x3F ||
                vec.RenderClass == 0x40)
            {
                endpointFlip =
                    !endpointFlip;
            }

            short local20;
            short perpendicular;

            if (vec.Orientation == 0 ||
                vec.Orientation == 1)
            {
                if (!endpointFlip)
                {
                    local20 =
                        OriginalRendererCore.Subtract16(
                            playerX,
                            vec.X1);
                }
                else
                {
                    local20 =
                        OriginalRendererCore.Wrap16(
                            OriginalRendererCore.Subtract16(
                                playerX,
                                vec.X2) +
                            0x80);
                }

                perpendicular =
                    OriginalRendererCore.Subtract16(
                        playerY,
                        vec.Y1);
            }
            else
            {
                if (!endpointFlip)
                {
                    local20 =
                        OriginalRendererCore.Subtract16(
                            vec.Y1,
                            playerY);
                }
                else
                {
                    local20 =
                        OriginalRendererCore.Wrap16(
                            OriginalRendererCore.Subtract16(
                                vec.Y2,
                                playerY) -
                            0x80);
                }

                perpendicular =
                    OriginalRendererCore.Subtract16(
                        vec.X1,
                        playerX);
            }

            int angle =
                OriginalTrigQ10.Normalize(
                    angleDegrees);

            int sinQ10 =
                trig.Sin(angle);

            int cosQ10 =
                trig.Cos(angle);

            bool cosineDivisor =
                UsesCosineDivisor(angle);

            int perpendicularProduct =
                perpendicular *
                (cosineDivisor
                    ? sinQ10
                    : cosQ10);

            short along =
                PerspectiveCoordinate(
                    vec.Orientation,
                    screenX,
                    spanYQ4,
                    perpendicularProduct,
                    angle,
                    trig);

            along =
                OriginalRendererCore.Wrap16(
                    along + local20);

            bool negate =
                vec.RenderClass != 2 &&
                (vec.Orientation == 0 ||
                 vec.Orientation == 3);

            if (negate)
            {
                along =
                    OriginalRendererCore.Negate16(
                        along);
            }

            return
                OriginalRendererCore.SelectTextureU(
                    vec,
                    screenX,
                    along,
                    textureWidth);
        }
    }
}