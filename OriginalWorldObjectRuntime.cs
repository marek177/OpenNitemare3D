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

        /// <summary>
        /// General non-extended OBJECT animation path: advance at most one
        /// frame when the absolute deadline is due, loop at frameCount and
        /// schedule now+interval. This is the safe path for SEQDEFs whose
        /// extension/alternative selector is zero.
        /// </summary>
        public static bool AdvanceSimpleAnimationIfDue(
            ref OriginalObjectRecord obj,
            OriginalObjectDefinitionRecord definition,
            uint now)
        {
            if (definition.Interval == 0 ||
                definition.FrameCount == 0 ||
                now < obj.RuntimeValue)
            {
                return false;
            }

            int next =
                unchecked((byte)obj.Component03) + 1;

            if (next >= definition.FrameCount)
                next = 0;

            obj.Component03 =
                unchecked((sbyte)next);

            obj.RuntimeValue =
                unchecked(
                    now +
                    definition.Interval);

            return true;
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
