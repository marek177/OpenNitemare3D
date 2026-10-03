using System;

namespace Nitemare3D
{
    /// <summary>
    /// Recovered common 28-byte world OBJECT behavior used by non-GUARD map
    /// objects. This intentionally covers only semantics closed by the binary
    /// audit: spawn fields, simple non-extended timestamped loops, collected
    /// object deactivation and the confirmed vertical anchors for classes
    /// 0x2E / 0x3B.
    /// </summary>
    public static class OriginalWorldObjectRuntime
    {
        public static bool ShouldUseGenericShell(byte propertyFlags)
        {
            return
                (propertyFlags &
                 OriginalMapTables.ObjectRuntimePresent) != 0 &&
                (propertyFlags &
                 OriginalMapTables.ObjectCreatesGuard) == 0;
        }

        public static void InitializeMapObject(
            ref OriginalObjectRecord obj,
            byte rawObjectId,
            byte variant,
            byte propertyFlags,
            byte objectClass,
            byte definitionId,
            short worldX,
            short worldY)
        {
            // Do not clear the whole struct here. OBJECT+0x18 is deliberately
            // persistent in the original when a slot is reused; callers decide
            // whether a pool reset is appropriate.
            obj.MapObjectId = rawObjectId;
            obj.Variant = variant;
            obj.Component02 = 0;
            obj.Component03 = 0;
            obj.DefinitionId = definitionId;
            obj.Flags = propertyFlags;
            obj.ObjectClass = objectClass;
            obj.GuardIndex = 0;
            obj.RuntimeValue = 0;
            obj.WorldX = worldX;
            obj.WorldY = worldY;
            obj.MapCellBinding =
                OriginalMapTables.CellBindingHandle(
                    worldX >> 6,
                    worldY >> 6);
            obj.Runtime1A = 0;
        }

        static ushort Alternative(
            OriginalObjectDefinitionRecord definition,
            int selector)
        {
            return definition.GetDirectionalSequenceA(
                selector & 7);
        }

        /// <summary>
        /// Exact DOS B17E directional OBJECT animation used by classes 0x2C/0x2D.
        ///
        /// phaseCount = frameCount/8 for class 0x2D, otherwise frameCount/4.
        /// localPhase is the current frame modulo phaseCount. When due, exactly
        /// one frame is advanced and the deadline becomes now+interval.
        ///
        /// BAAA supplies the player-relative octant. B17E then transforms it as:
        ///   group = (-(octant + 4)) & 7
        /// and ordinary directional objects reduce that to four groups via >>1;
        /// class 0x2D retains all eight groups.
        /// </summary>
        public static bool AdvanceDirectionalAnimation(
            ref OriginalObjectRecord obj,
            OriginalObjectDefinitionRecord definition,
            uint now,
            short playerWorldX,
            short playerWorldY)
        {
            bool eightGroups =
                obj.ObjectClass == 0x2D;

            if (!eightGroups &&
                obj.ObjectClass != 0x2C)
            {
                return false;
            }

            int phaseCount =
                definition.FrameCount >>
                (eightGroups ? 3 : 2);

            if (phaseCount <= 0)
                return false;

            bool due =
                now >= obj.RuntimeValue;

            if (due)
            {
                obj.Component03 =
                    unchecked(
                        (sbyte)(
                            unchecked((byte)obj.Component03) +
                            1));

                obj.RuntimeValue =
                    unchecked(
                        now +
                        definition.Interval);
            }

            int localPhase =
                unchecked((byte)obj.Component03) %
                phaseCount;

            byte octant =
                OriginalGuardDispatcher.ComputePlayerOctant(
                    eightGroups ? (byte)1 : (byte)0,
                    obj.WorldX,
                    obj.WorldY,
                    playerWorldX,
                    playerWorldY);

            int group =
                (-(octant + 4)) & 7;

            if (!eightGroups)
                group >>= 1;

            obj.Component03 =
                unchecked(
                    (sbyte)(
                        group * phaseCount +
                        localPhase));

            return due;
        }

        /// <summary>
        /// General timestamped world-OBJECT animation path from B22C:
        /// one due frame per call, simple loop when no alternatives exist,
        /// otherwise use OBJECT+0x02 as the 0..7 branch selector. Each branch
        /// word is low=start frame, high=length. On branch completion sample
        /// original RNG until a non-empty branch is selected.
        ///
        /// Classes 0x2C/0x2D use the separate directional-animation path and
        /// are deliberately not handled here.
        /// </summary>
        public static bool AdvancePresentationAnimationIfDue(
            ref OriginalObjectRecord obj,
            OriginalObjectDefinitionRecord definition,
            uint now,
            Func<ushort> nextRandom)
        {
            if (obj.ObjectClass == 0x2C ||
                obj.ObjectClass == 0x2D ||
                definition.Interval == 0 ||
                definition.FrameCount == 0 ||
                now < obj.RuntimeValue)
            {
                return false;
            }

            int next =
                unchecked((byte)obj.Component03) + 1;

            if (definition.ExtensionFlag == 0)
            {
                if (next >= definition.FrameCount)
                    next = 0;

                obj.Component03 =
                    unchecked((sbyte)next);
            }
            else
            {
                ushort branch =
                    Alternative(
                        definition,
                        unchecked((byte)obj.Component02));

                int start =
                    branch & 0xFF;

                int length =
                    branch >> 8;

                if (length == 0 ||
                    next >= start + length)
                {
                    if (nextRandom == null)
                    {
                        throw new InvalidOperationException(
                            "Extended OBJECT SEQDEF requires original-compatible RNG.");
                    }

                    int selector;
                    do
                    {
                        selector =
                            nextRandom() & 7;

                        branch =
                            Alternative(
                                definition,
                                selector);

                        start =
                            branch & 0xFF;

                        length =
                            branch >> 8;
                    }
                    while (length == 0);

                    obj.Component02 =
                        unchecked((sbyte)selector);

                    next =
                        start;
                }

                obj.Component03 =
                    unchecked((sbyte)next);
            }

            obj.RuntimeValue =
                unchecked(
                    now +
                    definition.Interval);

            return true;
        }

        // Compatibility name retained for existing direct tests/callers.
        public static bool AdvanceSimpleAnimationIfDue(
            ref OriginalObjectRecord obj,
            OriginalObjectDefinitionRecord definition,
            uint now)
        {
            return AdvancePresentationAnimationIfDue(
                ref obj,
                definition,
                now,
                () => 0);
        }

        /// <summary>
        /// Closed class-specific vertical/elevation anchors from BA20:
        /// class 0x2E = 64-frameHeight; class 0x3B = (64-frameHeight)/2.
        /// Other classes remain unchanged here because their adjustment helper
        /// is class-specific and not needed by the first pickup/scenery batch.
        /// </summary>
        public static bool UpdateKnownVerticalAnchor(
            ref OriginalObjectRecord obj,
            int frameHeight)
        {
            if (frameHeight < 0)
                frameHeight = 0;

            int value;

            switch (obj.ObjectClass)
            {
                case 0x2E:
                    value = 64 - frameHeight;
                    break;

                case 0x3B:
                    value = (64 - frameHeight) / 2;
                    break;

                default:
                    return false;
            }

            if (value < 0)
                value = 0;
            else if (value > byte.MaxValue)
                value = byte.MaxValue;

            obj.Runtime1A = (byte)value;
            return true;
        }

        public static void DeactivateCollected(
            ref OriginalObjectRecord obj)
        {
            obj.Flags =
                (byte)(
                    obj.Flags &
                    ~OriginalMapTables.ObjectRuntimePresent);
        }
    }
}
