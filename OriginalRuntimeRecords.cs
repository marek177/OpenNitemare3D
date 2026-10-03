using System.Runtime.InteropServices;

namespace Nitemare3D
{
    // Byte-exact clean-room mirrors of the verified Win16 runtime records.
    // Unknown/partial fields deliberately keep neutral names until their semantics close.

    [StructLayout(LayoutKind.Explicit, Pack = 1, Size = OriginalRuntime.ObjectRuntimeStride)]
    public struct OriginalObjectRecord
    {
        [FieldOffset(0x00)] public byte MapObjectId;
        [FieldOffset(0x01)] public byte Variant;
        [FieldOffset(0x02)] public sbyte Component02;
        [FieldOffset(0x03)] public sbyte Component03;
        [FieldOffset(0x04)] public byte DefinitionId;
        [FieldOffset(0x05)] public byte Flags;
        [FieldOffset(0x06)] public byte ObjectClass;
        [FieldOffset(0x07)] public byte GuardIndex;
        [FieldOffset(0x08)] public uint RuntimeValue;
        [FieldOffset(0x0C)] public uint MapCellBinding;
        [FieldOffset(0x10)] public short WorldX;
        [FieldOffset(0x12)] public short WorldY;
        [FieldOffset(0x14)] public short Spatial14;
        [FieldOffset(0x16)] public short Spatial16;
        [FieldOffset(0x18)] public short ProjectedBaseRow;
        [FieldOffset(0x1A)] public byte Runtime1A;
        [FieldOffset(0x1B)] public byte Unknown1B;
    }

    [StructLayout(LayoutKind.Explicit, Pack = 1, Size = OriginalRuntime.DoorRuntimeStride)]
    public struct OriginalDoorRuntimeRecord
    {
        // Original Win16 values are far pointers. The clean-room port may use
        // stable opaque handles here; behavioral code uses the C# references
        // retained by OriginalWallRuntime.PairedWall.
        [FieldOffset(0x00)] public uint FirstVectorRef;
        [FieldOffset(0x04)] public uint SecondVectorRef;
        [FieldOffset(0x08)] public uint MapCellBinding;
        [FieldOffset(0x0C)] public short State;
        [FieldOffset(0x0E)] public short Timer;
        [FieldOffset(0x10)] public short TargetX;
        [FieldOffset(0x12)] public short TargetY;
        [FieldOffset(0x14)] public byte Latch;
        [FieldOffset(0x15)] public byte Unknown15;
    }

    [StructLayout(LayoutKind.Explicit, Pack = 1, Size = OriginalRuntime.GuardRuntimeStride)]
    public struct OriginalGuardRecord
    {
        [FieldOffset(0x00)] public ushort DefinitionValue;
        [FieldOffset(0x02)] public uint Timestamp;
        [FieldOffset(0x06)] public short Timer;
        [FieldOffset(0x08)] public ushort ObjectSlot;
        [FieldOffset(0x0A)] public byte Strategy;
        [FieldOffset(0x0B)] public byte State;
        [FieldOffset(0x0C)] public byte NextState;
        [FieldOffset(0x0D)] public byte ObjectId;
        [FieldOffset(0x0E)] public byte DefinitionLookup;
        [FieldOffset(0x0F)] public byte Control;
        [FieldOffset(0x10)] public byte Strength;
        [FieldOffset(0x11)] public byte Octant;
        [FieldOffset(0x12)] public byte ResultOctant;
        [FieldOffset(0x13)] public sbyte MoveX;
        [FieldOffset(0x14)] public sbyte MoveY;
        [FieldOffset(0x15)] public sbyte VerticalBobStep;
        [FieldOffset(0x16)] public byte TransitionControl;
        [FieldOffset(0x17)] public byte Unknown17;
        [FieldOffset(0x18)] public byte Unknown18;
        [FieldOffset(0x19)] public byte Unknown19;
    }

    public enum OriginalGuardState : byte
    {
        AnimationTimer = 0x00,
        Delay = 0x01,
        Active02 = 0x02,
        Detection03 = 0x03,
        DetectionAttack04 = 0x04,
        Transition05 = 0x05,
        MoveThen03 = 0x06,
        Active07 = 0x07,
        Move08 = 0x08,
        DeathFinalize09 = 0x09,
        // Compatibility alias kept for callers written before the state-09 closure.
        SpecialAction09 = DeathFinalize09,
        NoLocalAction0A = 0x0A,
        LethalPlayerContact0B = 0x0B,
        NoLocalAction0B = LethalPlayerContact0B,
        Shared0C = 0x0C,
        Shared0D = 0x0D,
        CannonIdle0E = 0x0E,
        Conditional0E = CannonIdle0E,
        CannonCadence0F = 0x0F,
        Timed0F = CannonCadence0F,
        CannonAttack10 = 0x10,
        Timed10 = CannonAttack10,
        RecoverMove11 = 0x11,
        WaitAnimation12 = 0x12,
        Transition13 = 0x13,
        Periodic14 = 0x14,
        Pain15 = 0x15
    }
}
