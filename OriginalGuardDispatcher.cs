using System;

namespace Nitemare3D
{
    public enum OriginalGuardDispatchResult
    {
        NotHandled,
        Waiting,
        Transitioned,
        Moved,
        MovementBlocked,
        SoundPoint,
        Completed
    }

    public enum OriginalGuardHitTransition
    {
        MissingSequence,
        ReactionSkipped,
        Pain15,
        AnimationTo05,
        AnimationTo08,
        DeathAnimation
    }

    public struct OriginalGuardMovementResult
    {
        public bool XBlocked;
        public bool YBlocked;
        public bool PositionCommitted;
        public bool Bounced;
        public short AppliedX;
        public short AppliedY;
    }

    public struct OriginalDoorCollisionInfo
    {
        public bool Exists;
        public short State;
        public byte RenderClass;
        public byte Orientation;
        public short TargetX;
        public short TargetY;
    }

    public struct OriginalGuardCellCollisionResult
    {
        public bool Blocked;
        public bool DoorToggleRequested;
        public bool DoorLatchRequested;
        public bool EnteredRecoverMove11;
    }

    /// <summary>
    /// Executable-backed pieces of the Win16 1.10 GUARD dispatcher.
    /// Partial states stay deliberately unimplemented rather than receiving guessed behavior.
    /// </summary>
    public static class OriginalGuardDispatcher
    {
        // State 0x01: countdown, then state 0x02.
        public static OriginalGuardDispatchResult TickDelay(ref OriginalGuardRecord guard)
        {
            if (guard.State != (byte)OriginalGuardState.Delay)
                return OriginalGuardDispatchResult.NotHandled;

            if (guard.Timer > 0)
                guard.Timer--;

            if (guard.Timer <= 0)
            {
                guard.Timer = 0;
                guard.State = (byte)OriginalGuardState.Active02;
                return OriginalGuardDispatchResult.Transitioned;
            }

            return OriginalGuardDispatchResult.Waiting;
        }

        /// <summary>
        /// Exact state-0x00 sequence/timer scheduler from FUN_1010_7B56.
        /// The low byte of DefinitionValue is the first frame and the high byte
        /// is the frame count. The sequence loops while the timer counts down.
        /// </summary>
        public static OriginalGuardDispatchResult TickAnimationTimer(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj)
        {
            if (guard.State != (byte)OriginalGuardState.AnimationTimer)
                return OriginalGuardDispatchResult.NotHandled;

            int firstFrame = guard.DefinitionValue & 0xFF;
            int frameCount = (guard.DefinitionValue >> 8) & 0xFF;
            int frame = unchecked((byte)obj.Component03);
            frame = (frame + 1) & 0xFF;

            if (firstFrame + frameCount <= frame)
                frame = firstFrame;

            obj.Component03 = unchecked((sbyte)(byte)frame);
            guard.Timer--;

            if (guard.Timer <= 0)
            {
                guard.State = guard.NextState;
                return OriginalGuardDispatchResult.Transitioned;
            }

            return OriginalGuardDispatchResult.Waiting;
        }

        /// <summary>
        /// Exact state-0x12 tail: advance toward the last sequence frame,
        /// decay OBJECT+0x1A by five, count the timer down only while positive,
        /// then return through nextState once both gates reach zero.
        /// </summary>
        public static OriginalGuardDispatchResult TickWaitAnimation12(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj)
        {
            if (guard.State != (byte)OriginalGuardState.WaitAnimation12)
                return OriginalGuardDispatchResult.NotHandled;

            int firstFrame = guard.DefinitionValue & 0xFF;
            int frameCount = (guard.DefinitionValue >> 8) & 0xFF;
            int lastFrame = firstFrame + frameCount - 1;
            int frame = unchecked((byte)obj.Component03);

            if (frame < lastFrame)
                obj.Component03 = unchecked((sbyte)(byte)(frame + 1));

            obj.Runtime1A = obj.Runtime1A > 5
                ? (byte)(obj.Runtime1A - 5)
                : (byte)0;

            if (guard.Timer > 0)
                guard.Timer--;

            if (guard.Timer == 0 && obj.Runtime1A == 0)
            {
                guard.State = guard.NextState;
                return OriginalGuardDispatchResult.Transitioned;
            }

            return OriginalGuardDispatchResult.Waiting;
        }

        // Compatibility completion helper retained for call sites that already
        // know the original sequence/timer gate has completed.
        public static OriginalGuardDispatchResult CompleteDeferredState(
            ref OriginalGuardRecord guard)
        {
            if (guard.State != (byte)OriginalGuardState.AnimationTimer &&
                guard.State != (byte)OriginalGuardState.WaitAnimation12)
            {
                return OriginalGuardDispatchResult.NotHandled;
            }

            guard.State = guard.NextState;
            return OriginalGuardDispatchResult.Completed;
        }

        /// <summary>
        /// Exact FUN_1010_762C packed-sequence initializer. The sequence word packs
        /// the first animation frame in its low byte and frame-count/timer data in
        /// its high byte. State 0x02/0x03/0x04 use this with class-table offsets
        /// +0x34/+0x36/+0x38 and next states 3/4/5 respectively.
        /// </summary>
        public static OriginalGuardDispatchResult BeginPackedSequence(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj,
            ushort sequenceValue,
            byte state,
            byte nextState)
        {
            guard.DefinitionValue = sequenceValue;
            obj.Component03 = unchecked((sbyte)(byte)sequenceValue);
            guard.Timer = (short)(((sequenceValue >> 8) & 0xFF) - 1);
            guard.NextState = nextState;
            guard.State = state;
            return OriginalGuardDispatchResult.Transitioned;
        }

        public static OriginalGuardDispatchResult BeginState02AlertSequence(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj,
            ushort classSequence34)
        {
            if (guard.State != (byte)OriginalGuardState.Active02)
                return OriginalGuardDispatchResult.NotHandled;

            // The original calls the class-specific alert SFX selector immediately
            // before this sequence initializer.
            return BeginPackedSequence(
                ref guard,
                ref obj,
                classSequence34,
                (byte)OriginalGuardState.AnimationTimer,
                (byte)OriginalGuardState.Detection03);
        }

        public static OriginalGuardDispatchResult BeginState03AttackSequence(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj,
            ushort classSequence36)
        {
            if (guard.State != (byte)OriginalGuardState.Detection03)
                return OriginalGuardDispatchResult.NotHandled;

            return BeginPackedSequence(
                ref guard,
                ref obj,
                classSequence36,
                (byte)OriginalGuardState.AnimationTimer,
                (byte)OriginalGuardState.DetectionAttack04);
        }

        public static OriginalGuardDispatchResult BeginState04RecoverySequence(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj,
            ushort classSequence38)
        {
            if (guard.State != (byte)OriginalGuardState.DetectionAttack04)
                return OriginalGuardDispatchResult.NotHandled;

            return BeginPackedSequence(
                ref guard,
                ref obj,
                classSequence38,
                (byte)OriginalGuardState.AnimationTimer,
                (byte)OriginalGuardState.Transition05);
        }

        /// <summary>
        /// Exact range/FOV prefilter from FUN_1010_7494 before the D50A line trace.
        /// World positions are converted to 64-unit tile coordinates. The original
        /// rejects targets farther than 8 tiles independently on either axis.
        /// When ignoreFacing is false, the original 8-bit octant mask is applied.
        /// </summary>
        public static bool GuardPerceptionPrefilter(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj,
            short playerWorldX,
            short playerWorldY,
            bool ignoreFacing)
        {
            int guardTileX = obj.WorldX >> 6;
            int guardTileY = obj.WorldY >> 6;
            int playerTileX = playerWorldX >> 6;
            int playerTileY = playerWorldY >> 6;

            int dx = playerTileX - guardTileX;
            int dy = playerTileY - guardTileY;

            int absX = Math.Abs(dx);
            int absY = Math.Abs(dy);
            if (absX > 8 || absY > 8)
                return false;

            if (ignoreFacing)
                return true;

            int octant = guard.Octant & 7;
            int facingMask =
                (1 << ((octant - 1) & 7)) |
                (1 << octant) |
                (1 << ((octant + 1) & 7));

            int dominantAxisMask = absX < absY ? 0x99 : 0x66;
            int candidateMask = facingMask & dominantAxisMask;

            candidateMask &= dy >= 0 ? 0x3C : 0xC3;
            int xSideMask = dx >= 0 ? 0x0F : 0xF0;

            return (candidateMask & xSideMask) != 0;
        }

        /// <summary>
        /// Exact stepping core of FUN_1010_D50A. The callback is invoked only for
        /// intermediate cells, never for the destination cell, matching the original
        /// target comparison order. Returning true from isIntermediateBlocked aborts
        /// the trace. maxSteps is 8 for GUARD perception.
        /// </summary>
        public static bool TraceGuardGridLine(
            int startX,
            int startY,
            int deltaX,
            int deltaY,
            int maxSteps,
            bool secondaryCellChecks,
            Func<int, int, bool, bool> isIntermediateBlocked)
        {
            int stepX = deltaX > 0 ? 1 : -1;
            int stepY = deltaY > 0 ? 1 : -1;

            int targetX = startX + deltaX;
            int targetY = startY + deltaY;

            int absX = Math.Abs(deltaX);
            int absY = Math.Abs(deltaY);

            bool xMajor = absY < absX;
            int error;
            int straightAdjust;
            int diagonalAdjust;

            if (xMajor)
            {
                error = 2 * absY - absX;
                straightAdjust = 2 * absY;
                diagonalAdjust = 2 * (absY - absX);
            }
            else
            {
                error = 2 * absX - absY;
                straightAdjust = 2 * absX;
                diagonalAdjust = 2 * (absX - absY);
            }

            int x = startX;
            int y = startY;

            if (maxSteps <= 0)
                return false;

            for (int step = 0; step < maxSteps; step++)
            {
                if (xMajor)
                    x += stepX;
                else
                    y += stepY;

                if (error < 0)
                {
                    error += straightAdjust;
                }
                else
                {
                    error += diagonalAdjust;
                    if (xMajor)
                        y += stepY;
                    else
                        x += stepX;
                }

                if (x == targetX && y == targetY)
                    return true;

                if (isIntermediateBlocked != null &&
                    isIntermediateBlocked(x, y, secondaryCellChecks))
                {
                    return false;
                }
            }

            return false;
        }

        /// <summary>
        /// Executable-backed FUN_1010_7494 wrapper with the D50A map trace supplied
        /// by the caller. The trace callback receives start tile, signed deltas,
        /// the fixed maximum of 8 steps and the original secondary-cell flag.
        /// </summary>
        public static bool EvaluateGuardPerception(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj,
            short playerWorldX,
            short playerWorldY,
            bool secondaryCellChecks,
            bool ignoreFacing,
            Func<int, int, int, int, int, bool, bool> lineTrace)
        {
            if (!GuardPerceptionPrefilter(
                    ref guard,
                    ref obj,
                    playerWorldX,
                    playerWorldY,
                    ignoreFacing))
            {
                return false;
            }

            if (lineTrace == null)
                return false;

            int guardTileX = obj.WorldX >> 6;
            int guardTileY = obj.WorldY >> 6;
            int playerTileX = playerWorldX >> 6;
            int playerTileY = playerWorldY >> 6;

            return lineTrace(
                guardTileX,
                guardTileY,
                playerTileX - guardTileX,
                playerTileY - guardTileY,
                8,
                secondaryCellChecks);
        }

        /// <summary>
        /// Recovered FUN_1010_7594 decision tail. The helper caches LOS/perception
        /// at +0x17 and one-tile proximity at +0x18. +0x16 selects which result is
        /// used by states 0x03 and 0x04: 0 = one-tile proximity, 1/2 = perception.
        /// Values above 2 are deliberately rejected instead of reproducing the
        /// original uninitialized local-byte fallthrough.
        /// </summary>
        public static bool TryEvaluateAttackGate(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj,
            short playerWorldX,
            short playerWorldY,
            bool perceptionSucceeded,
            out bool attackEligible)
        {
            guard.Unknown17 = perceptionSucceeded ? (byte)1 : (byte)0;

            int dx = playerWorldX - obj.WorldX;
            int dy = playerWorldY - obj.WorldY;
            bool withinOneTile =
                Math.Abs(dx) <= OriginalRuntime.WorldUnitsPerTile &&
                Math.Abs(dy) <= OriginalRuntime.WorldUnitsPerTile;

            guard.Unknown18 = withinOneTile ? (byte)1 : (byte)0;

            switch (guard.TransitionControl)
            {
                case 0:
                    attackEligible = withinOneTile;
                    return true;

                case 1:
                case 2:
                    attackEligible = perceptionSucceeded;
                    return true;

                default:
                    attackEligible = false;
                    return false;
            }
        }

        public static bool PackedSequenceHasFrames(ushort packedSequence)
        {
            return (packedSequence & 0xFF00) != 0;
        }

        static bool TrySelectSequence(
            OriginalObjectDefinitionRecord definition,
            bool death,
            byte resultOctant,
            Func<ushort> nextRandom,
            out int selector,
            out ushort packedSequence)
        {
            selector = 0;
            packedSequence = 0;

            ushort slot7;
            bool slot7Valid = death
                ? definition.TryGetDeathSequence(7, out slot7)
                : definition.TryGetReactionSequence(7, out slot7);

            if (resultOctant == 0 && slot7Valid)
            {
                selector = 7;
                packedSequence = slot7;
                return true;
            }

            if (nextRandom == null)
                return false;

            bool anyRandomSlot = false;
            for (int i = 0; i < 7; i++)
            {
                ushort candidate;
                bool valid = death
                    ? definition.TryGetDeathSequence(i, out candidate)
                    : definition.TryGetReactionSequence(i, out candidate);
                if (valid)
                {
                    anyRandomSlot = true;
                    break;
                }
            }

            if (!anyRandomSlot)
                return false;

            while (true)
            {
                selector = nextRandom() % 7;
                bool valid = death
                    ? definition.TryGetDeathSequence(selector, out packedSequence)
                    : definition.TryGetReactionSequence(selector, out packedSequence);
                if (valid)
                    return true;
            }
        }

        public static bool TrySelectReactionSequence(
            OriginalObjectDefinitionRecord definition,
            byte resultOctant,
            Func<ushort> nextRandom,
            out int selector,
            out ushort packedSequence)
        {
            return TrySelectSequence(
                definition,
                false,
                resultOctant,
                nextRandom,
                out selector,
                out packedSequence);
        }

        public static bool TrySelectDeathSequence(
            OriginalObjectDefinitionRecord definition,
            byte resultOctant,
            Func<ushort> nextRandom,
            out int selector,
            out ushort packedSequence)
        {
            return TrySelectSequence(
                definition,
                true,
                resultOctant,
                nextRandom,
                out selector,
                out packedSequence);
        }

        public static OriginalGuardDispatchResult BeginDeathSequence(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj,
            ushort packedSequence)
        {
            byte state = obj.Runtime1A > 0
                ? (byte)OriginalGuardState.WaitAnimation12
                : (byte)OriginalGuardState.AnimationTimer;

            return BeginPackedSequence(
                ref guard,
                ref obj,
                packedSequence,
                state,
                (byte)OriginalGuardState.DeathFinalize09);
        }

        public static OriginalGuardDispatchResult BeginPainReaction15(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj,
            ushort packedSequence)
        {
            guard.DefinitionValue = packedSequence;
            obj.Component03 = unchecked((sbyte)(byte)packedSequence);

            if (guard.State != (byte)OriginalGuardState.AnimationTimer)
                guard.NextState = guard.State;

            guard.State = (byte)OriginalGuardState.Pain15;
            return OriginalGuardDispatchResult.Transitioned;
        }

        public static OriginalGuardDispatchResult BeginReactionAnimation(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj,
            ushort packedSequence,
            OriginalGuardState nextState)
        {
            return BeginPackedSequence(
                ref guard,
                ref obj,
                packedSequence,
                (byte)OriginalGuardState.AnimationTimer,
                (byte)nextState);
        }

        public static OriginalGuardHitTransition BeginNonLethalHitTransition(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj,
            OriginalObjectDefinitionRecord definition,
            Func<ushort> nextRandom,
            bool perceptionSucceeded)
        {
            // FUN_1010_80F8 writes these before all strategy/state branches.
            guard.ResultOctant = 8;

            // Strategy 4 exits before B9B2 and therefore consumes no selector RNG.
            if (guard.Strategy == 4)
                return OriginalGuardHitTransition.ReactionSkipped;

            if (!TrySelectReactionSequence(
                    definition,
                    guard.ResultOctant,
                    nextRandom,
                    out _,
                    out ushort reactionSequence))
            {
                return OriginalGuardHitTransition.MissingSequence;
            }

            // Strategy 2 has a dedicated state-0 animation returning to state 8.
            if (guard.Strategy == 2)
            {
                BeginReactionAnimation(
                    ref guard,
                    ref obj,
                    reactionSequence,
                    OriginalGuardState.Move08);
                return OriginalGuardHitTransition.AnimationTo08;
            }

            // States 3, 4 and 0x0B consume the selector but do not enter a
            // local reaction animation in FUN_1010_80F8.
            if (guard.State == (byte)OriginalGuardState.Detection03 ||
                guard.State == (byte)OriginalGuardState.DetectionAttack04 ||
                guard.State == (byte)OriginalGuardState.LethalPlayerContact0B)
            {
                return OriginalGuardHitTransition.ReactionSkipped;
            }

            // States 7, 8 and 0x15 refresh the perception cache and run the
            // selected reaction sequence through state 0, returning to state 5.
            if (guard.State == (byte)OriginalGuardState.Active07 ||
                guard.State == (byte)OriginalGuardState.Move08 ||
                guard.State == (byte)OriginalGuardState.Pain15)
            {
                guard.Unknown17 = perceptionSucceeded ? (byte)1 : (byte)0;
                BeginReactionAnimation(
                    ref guard,
                    ref obj,
                    reactionSequence,
                    OriginalGuardState.Transition05);
                return OriginalGuardHitTransition.AnimationTo05;
            }

            BeginPainReaction15(
                ref guard,
                ref obj,
                reactionSequence);
            return OriginalGuardHitTransition.Pain15;
        }

        public static OriginalGuardHitTransition BeginLethalHitTransition(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj,
            OriginalObjectDefinitionRecord definition,
            Func<ushort> nextRandom)
        {
            guard.Strength = 0;

            if (!TrySelectDeathSequence(
                    definition,
                    guard.ResultOctant,
                    nextRandom,
                    out _,
                    out ushort deathSequence))
            {
                return OriginalGuardHitTransition.MissingSequence;
            }

            BeginDeathSequence(ref guard, ref obj, deathSequence);
            return OriginalGuardHitTransition.DeathAnimation;
        }

        public static void ApplyDraculaPhase2Reset(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj,
            byte batDefinitionId)
        {
            obj.DefinitionId = batDefinitionId;
            obj.ObjectClass = OriginalRuntime.DraculaBatPhase2Class;
            obj.Runtime1A = 0x23;

            guard.Strength = OriginalRuntime.GuardInitialStrength;
            guard.State = (byte)OriginalGuardState.Move08;
            guard.NextState = (byte)OriginalGuardState.Active02;
            guard.Timer = 1;
        }

        /// <summary>
        /// Exact state-0x15 pain scheduler. It advances to the final selected
        /// reaction frame, then returns through nextState.
        /// </summary>
        public static OriginalGuardDispatchResult TickPainReaction(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj)
        {
            if (guard.State != (byte)OriginalGuardState.Pain15)
                return OriginalGuardDispatchResult.NotHandled;

            int firstFrame = guard.DefinitionValue & 0xFF;
            int frameCount = (guard.DefinitionValue >> 8) & 0xFF;
            int lastFrame = firstFrame + frameCount - 1;
            int frame = unchecked((byte)obj.Component03);

            if (lastFrame <= frame)
            {
                guard.State = guard.NextState;
                return OriginalGuardDispatchResult.Transitioned;
            }

            obj.Component03 = unchecked((sbyte)(byte)(frame + 1));
            return OriginalGuardDispatchResult.Waiting;
        }

        // Compatibility completion helper retained for callers that already
        // know the selected pain sequence reached its final frame.
        public static OriginalGuardDispatchResult CompletePainReaction(
            ref OriginalGuardRecord guard)
        {
            if (guard.State != (byte)OriginalGuardState.Pain15)
                return OriginalGuardDispatchResult.NotHandled;

            guard.State = guard.NextState;
            return OriginalGuardDispatchResult.Completed;
        }

        /// <summary>
        /// Confirmed state-7 / strategy-3 transition helper.
        /// randomValue must be a sample from the original-compatible RNG path.
        /// </summary>
        public static void EnterStrategy3TimedMove(
            ref OriginalGuardRecord guard,
            ushort randomValue)
        {
            guard.Timer = (short)((randomValue % 0x50) + 8);
            guard.State = (byte)OriginalGuardState.Transition13;

            GetState13Movement(guard.Octant, out sbyte moveX, out sbyte moveY);
            guard.MoveX = moveX;
            guard.MoveY = moveY;
        }

        /// <summary>
        /// Exact recovered facing-to-cardinal movement table for state 0x13.
        /// Values are world units per movement update.
        /// </summary>
        public static void GetState13Movement(
            byte facing,
            out sbyte moveX,
            out sbyte moveY)
        {
            switch (facing & 7)
            {
                case 0:
                case 7:
                    moveX = 0;
                    moveY = -8;
                    break;
                case 1:
                case 2:
                    moveX = 8;
                    moveY = 0;
                    break;
                case 3:
                case 4:
                    moveX = 0;
                    moveY = 8;
                    break;
                default: // 5, 6
                    moveX = -8;
                    moveY = 0;
                    break;
            }
        }

        /// <summary>
        /// Advances confirmed state 0x13 control flow.
        /// The occupancy callback receives candidate world coordinates and should
        /// return true only when the candidate map cell/occupancy permits movement.
        /// </summary>
        public static OriginalGuardDispatchResult TickState13(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj,
            Func<short, short, bool> canMoveTo)
        {
            if (guard.State != (byte)OriginalGuardState.Transition13)
                return OriginalGuardDispatchResult.NotHandled;

            // The recovered helper transitions only when called with timer already 0.
            if (guard.Timer <= 0)
            {
                guard.Timer = 0;
                guard.Strategy = 0;
                guard.State = (byte)OriginalGuardState.Active02;
                return OriginalGuardDispatchResult.Transitioned;
            }

            guard.Timer--;

            if (guard.Timer == 8)
            {
                return OriginalGuardDispatchResult.SoundPoint;
            }

            // nextTimer < 8 performs a movement attempt, including 1 -> 0.
            // Therefore a state-13 sequence performs eight possible 8-unit moves.
            if (guard.Timer < 8)
            {
                short candidateX = (short)(obj.WorldX + guard.MoveX);
                short candidateY = (short)(obj.WorldY + guard.MoveY);

                if (canMoveTo != null && !canMoveTo(candidateX, candidateY))
                    return OriginalGuardDispatchResult.MovementBlocked;

                obj.WorldX = candidateX;
                obj.WorldY = candidateY;
                return OriginalGuardDispatchResult.Moved;
            }

            return OriginalGuardDispatchResult.Waiting;
        }

        public static byte ComputePlayerOctant(
            byte control,
            short guardWorldX,
            short guardWorldY,
            short playerWorldX,
            short playerWorldY)
        {
            int deltaX = playerWorldX - guardWorldX;
            int deltaY = playerWorldY - guardWorldY;
            int absX = Math.Abs(deltaX);
            int absY = Math.Abs(deltaY);

            if (control != 0)
            {
                if (absY * 2 < absX)
                    return deltaX > 0 ? (byte)2 : (byte)6;

                if (absX * 2 < absY)
                    return deltaY > 0 ? (byte)4 : (byte)0;

                if (deltaX > 0)
                    return deltaY > 0 ? (byte)3 : (byte)1;

                return deltaY > 0 ? (byte)5 : (byte)7;
            }

            if (deltaX < 0)
            {
                if (deltaY < 0)
                    return absY > absX ? (byte)7 : (byte)6;

                return absY > absX ? (byte)4 : (byte)5;
            }

            if (deltaY < 0)
                return absY > absX ? (byte)0 : (byte)1;

            return absY <= absX ? (byte)2 : (byte)3;
        }

        public static byte ComputeResultOctant(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj,
            short playerWorldX,
            short playerWorldY)
        {
            byte playerOctant = ComputePlayerOctant(
                guard.Control,
                obj.WorldX,
                obj.WorldY,
                playerWorldX,
                playerWorldY);

            int baseOffset = guard.Control == 0 ? 3 : 4;
            return (byte)((baseOffset + guard.Octant - playerOctant) & 7);
        }

        public static ushort GetDirectionalSequence(
            OriginalObjectDefinitionRecord definition,
            byte state,
            byte strategy,
            byte resultOctant)
        {
            int selector = resultOctant & 7;

            if (state == (byte)OriginalGuardState.MoveThen03 &&
                strategy != 2)
            {
                return definition.GetDirectionalSequenceC(selector);
            }

            if (state == (byte)OriginalGuardState.Move08 ||
                state == (byte)OriginalGuardState.Timed10 ||
                state == (byte)OriginalGuardState.RecoverMove11)
            {
                return definition.GetDirectionalSequenceB(selector);
            }

            return definition.GetDirectionalSequenceA(selector);
        }

        /// <summary>
        /// Recovered FUN_1010_6EE0 after the target-octant calculation.
        /// forceRefresh corresponds to the original fifth parameter.
        /// </summary>
        public static OriginalGuardDispatchResult RefreshDirectionalSequence(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj,
            OriginalObjectDefinitionRecord definition,
            short playerWorldX,
            short playerWorldY,
            bool forceRefresh)
        {
            byte resultOctant = ComputeResultOctant(
                ref guard,
                ref obj,
                playerWorldX,
                playerWorldY);

            if (!forceRefresh && guard.ResultOctant == resultOctant)
                return OriginalGuardDispatchResult.Waiting;

            guard.ResultOctant = resultOctant;
            ushort sequence = GetDirectionalSequence(
                definition,
                guard.State,
                guard.Strategy,
                resultOctant);

            guard.DefinitionValue = sequence;
            obj.Component03 = unchecked((sbyte)(byte)sequence);
            return OriginalGuardDispatchResult.Transitioned;
        }

        public static void UpdateOctantFromMovement(
            ref OriginalGuardRecord guard,
            int moveX,
            int moveY)
        {
            if (moveX > 0)
            {
                guard.Octant = moveY < 0
                    ? (byte)1
                    : moveY > 0
                        ? (byte)3
                        : (byte)2;
                return;
            }

            if (moveX < 0)
            {
                guard.Octant = moveY < 0
                    ? (byte)7
                    : moveY > 0
                        ? (byte)5
                        : (byte)6;
                return;
            }

            if (moveY < 0)
                guard.Octant = 0;
            else if (moveY > 0)
                guard.Octant = 4;
        }

        static sbyte SignedStepFromDelta(int delta)
        {
            if (delta < 0) return -8;
            if (delta > 0) return 8;
            return 0;
        }

        static OriginalGuardDispatchResult PlanPlayerPursuitMovement(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj,
            short playerWorldX,
            short playerWorldY,
            byte difficulty,
            Func<ushort> nextRandom)
        {
            if (nextRandom == null)
                throw new ArgumentNullException(nameof(nextRandom));

            // FUN_76FC uses signed divide-by-32 and therefore truncates toward zero.
            int deltaX32 = (playerWorldX - obj.WorldX) / 32;
            int deltaY32 = (playerWorldY - obj.WorldY) / 32;

            int directionChoice =
                nextRandom() & (guard.Unknown17 == 0 ? 3 : 7);

            if (directionChoice == 0)
            {
                if (deltaX32 == 0)
                    guard.MoveX = 8;
                if (deltaY32 == 0)
                    guard.MoveY = 8;
            }
            else if (directionChoice == 1)
            {
                if (deltaX32 == 0)
                    guard.MoveX = -8;
                if (deltaY32 == 0)
                    guard.MoveY = -8;
            }
            else
            {
                guard.MoveX = SignedStepFromDelta(deltaX32);
                guard.MoveY = SignedStepFromDelta(deltaY32);
            }

            if (guard.Unknown18 != 0)
            {
                guard.Timer = 8;
            }
            else if (guard.Unknown17 == 0)
            {
                guard.Timer = 0x18;
            }
            else
            {
                guard.Timer = (short)(nextRandom() % 8 + 8);

                if (difficulty == 2)
                    guard.Timer = (short)(guard.Timer >> 1);
                else if (difficulty == 0)
                    guard.Timer = (short)(guard.Timer << 1);
            }

            guard.State = (byte)OriginalGuardState.MoveThen03;
            UpdateOctantFromMovement(
                ref guard,
                guard.MoveX,
                guard.MoveY);
            return OriginalGuardDispatchResult.Transitioned;
        }

        /// <summary>
        /// Recovered strategy-0 slice of FUN_1010_76FC.
        /// </summary>
        public static OriginalGuardDispatchResult PlanStrategy0Movement(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj,
            short playerWorldX,
            short playerWorldY,
            byte difficulty,
            Func<ushort> nextRandom)
        {
            if (guard.Strategy != 0)
                return OriginalGuardDispatchResult.NotHandled;

            return PlanPlayerPursuitMovement(
                ref guard,
                ref obj,
                playerWorldX,
                playerWorldY,
                difficulty,
                nextRandom);
        }

        /// <summary>
        /// Recovered strategy-1 / FLEE branch of FUN_1010_76FC.
        /// Strength >= 0x7F falls back to the normal player-pursuit planner.
        /// Below 0x7F, FUN_1394 supplies the nearest reachable DOOR controller.
        /// The exact controller target coordinates (+0x10/+0x12) are supplied by
        /// the caller; when no door is found the previous movement vector is kept.
        /// </summary>
        public static OriginalGuardDispatchResult PlanStrategy1Movement(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj,
            short playerWorldX,
            short playerWorldY,
            byte difficulty,
            Func<ushort> nextRandom,
            bool retreatDoorFound,
            short retreatTargetWorldX,
            short retreatTargetWorldY)
        {
            if (guard.Strategy != 1)
                return OriginalGuardDispatchResult.NotHandled;

            if (guard.Strength >= 0x7F)
            {
                return PlanPlayerPursuitMovement(
                    ref guard,
                    ref obj,
                    playerWorldX,
                    playerWorldY,
                    difficulty,
                    nextRandom);
            }

            if (retreatDoorFound)
            {
                // FUN_76FC aims 32 world units inside the selected DOOR target.
                int dx = retreatTargetWorldX - obj.WorldX + 0x20;
                int dy = retreatTargetWorldY - obj.WorldY + 0x20;

                guard.MoveX = SignedStepFromDelta(dx);
                guard.MoveY = SignedStepFromDelta(dy);
            }

            guard.Timer = 0x10;
            guard.State = (byte)OriginalGuardState.MoveThen03;
            UpdateOctantFromMovement(
                ref guard,
                guard.MoveX,
                guard.MoveY);
            return OriginalGuardDispatchResult.Transitioned;
        }

        public static OriginalGuardDispatchResult PlanStrategy2Movement(
            ref OriginalGuardRecord guard,
            Func<ushort> nextRandom)
        {
            if (guard.Strategy != 2)
                return OriginalGuardDispatchResult.NotHandled;
            if (nextRandom == null)
                throw new ArgumentNullException(nameof(nextRandom));

            guard.Timer = (short)(nextRandom() % 8 + 8);
            guard.State = (byte)OriginalGuardState.MoveThen03;
            UpdateOctantFromMovement(
                ref guard,
                guard.MoveX,
                guard.MoveY);
            return OriginalGuardDispatchResult.Transitioned;
        }

        /// <summary>
        /// Strategies 3 and 4 take the common FUN_76FC tail without replacing
        /// timer or movement vector; the existing vector is simply promoted to state 6.
        /// </summary>
        public static OriginalGuardDispatchResult PlanStrategyCurrentVectorMovement(
            ref OriginalGuardRecord guard)
        {
            if (guard.Strategy < 3)
                return OriginalGuardDispatchResult.NotHandled;

            guard.State = (byte)OriginalGuardState.MoveThen03;
            UpdateOctantFromMovement(
                ref guard,
                guard.MoveX,
                guard.MoveY);
            return OriginalGuardDispatchResult.Transitioned;
        }

        /// <summary>
        /// Strategy dispatch portion of FUN_1010_76FC. This intentionally stops
        /// before the common forced sequence refresh and immediate FUN_71DC movement
        /// attempt so those independently recovered stages remain testable.
        /// </summary>
        public static OriginalGuardDispatchResult PlanMovement76FC(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj,
            short playerWorldX,
            short playerWorldY,
            byte difficulty,
            Func<ushort> nextRandom,
            bool retreatDoorFound,
            short retreatTargetWorldX,
            short retreatTargetWorldY)
        {
            switch (guard.Strategy)
            {
                case 0:
                    return PlanStrategy0Movement(
                        ref guard,
                        ref obj,
                        playerWorldX,
                        playerWorldY,
                        difficulty,
                        nextRandom);

                case 1:
                    return PlanStrategy1Movement(
                        ref guard,
                        ref obj,
                        playerWorldX,
                        playerWorldY,
                        difficulty,
                        nextRandom,
                        retreatDoorFound,
                        retreatTargetWorldX,
                        retreatTargetWorldY);

                case 2:
                    return PlanStrategy2Movement(
                        ref guard,
                        nextRandom);

                default:
                    return PlanStrategyCurrentVectorMovement(ref guard);
            }
        }

        public static bool TickVerticalBob(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj)
        {
            if (obj.ObjectClass != 0x08 &&
                obj.ObjectClass != OriginalRuntime.DraculaBatPhase2Class &&
                obj.ObjectClass != 0x1A)
            {
                return false;
            }

            if (guard.VerticalBobStep == 0)
                guard.VerticalBobStep = 1;

            int next = obj.Runtime1A + guard.VerticalBobStep;
            obj.Runtime1A = unchecked((byte)next);

            if ((sbyte)obj.Runtime1A <= 10)
            {
                obj.Runtime1A = 10;
                guard.VerticalBobStep =
                    unchecked((sbyte)-guard.VerticalBobStep);
            }

            if (obj.Runtime1A >= 0x23)
            {
                obj.Runtime1A = 0x23;
                guard.VerticalBobStep =
                    unchecked((sbyte)-guard.VerticalBobStep);
            }

            return true;
        }

        public static bool MovementCandidateTouchesPlayer(
            short candidateX,
            short candidateY,
            short playerWorldX,
            short playerWorldY)
        {
            return Math.Abs(candidateX - playerWorldX) < 0x2A &&
                   Math.Abs(candidateY - playerWorldY) < 0x2A;
        }

        static short DirectionPadding(sbyte component)
        {
            if (component < 0) return -0x10;
            if (component > 0) return 0x10;
            return 0;
        }

        static void AdvanceMovementFrame(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj)
        {
            int firstFrame = guard.DefinitionValue & 0xFF;
            int frameCount = (guard.DefinitionValue >> 8) & 0xFF;
            int frame = (unchecked((byte)obj.Component03) + 1) & 0xFF;

            if (firstFrame + frameCount <= frame)
                frame = firstFrame;

            obj.Component03 = unchecked((sbyte)(byte)frame);
        }

        /// <summary>
        /// Decision/side-effect core of FUN_1010_700A after the candidate world
        /// coordinate has been mapped to its MAP cell. The caller supplies the
        /// already-derived MAP property bytes and, for door-family cells, the
        /// matching 22-byte DOOR-controller facts.
        /// </summary>
        public static OriginalGuardCellCollisionResult EvaluateMovementCell700A(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj,
            short candidateWorldX,
            short candidateWorldY,
            short playerWorldX,
            short playerWorldY,
            byte wallFlags,
            byte objectFlags,
            OriginalDoorCollisionInfo door,
            OriginalObjectDefinitionRecord definition)
        {
            var result = new OriginalGuardCellCollisionResult();

            // First branch of FUN_700A: the player is an immediate movement block.
            if (MovementCandidateTouchesPlayer(
                    candidateWorldX,
                    candidateWorldY,
                    playerWorldX,
                    playerWorldY))
            {
                result.Blocked = true;
                return result;
            }

            // Object-property bit 0x02 blocks before any door handling.
            if ((objectFlags & OriginalMapTables.ObjectBlocksMovementOrLos) != 0)
            {
                result.Blocked = true;
                return result;
            }

            if ((wallFlags & OriginalMapTables.WallDynamicDoor) != 0)
            {
                if (!door.Exists)
                {
                    // The original expects every bit-08 MAP cell to have a DOOR
                    // controller. Treat a broken bridge as blocked.
                    result.Blocked = true;
                    return result;
                }

                if (door.State != 1)
                {
                    // FUN_1476: only states 0 and 4 are passable.
                    result.Blocked = door.State != 0 && door.State != 4;
                    return result;
                }

                // Closed state-1 classes 0x33..0x3C are not activated by GUARDs.
                if (door.RenderClass >= 0x33 && door.RenderClass <= 0x3C)
                {
                    result.Blocked = true;
                    return result;
                }

                if (guard.State == (byte)OriginalGuardState.MoveThen03)
                    result.DoorLatchRequested = true;

                if (guard.Strategy == 1)
                {
                    guard.Timer = 0x20;
                    guard.MoveX = 0;
                    guard.MoveY = 0;

                    if (door.Orientation == 2)
                    {
                        guard.MoveX = obj.WorldX < candidateWorldX
                            ? (sbyte)8
                            : obj.WorldX > candidateWorldX
                                ? (sbyte)-8
                                : (sbyte)0;
                        obj.WorldY = (short)(door.TargetY + 0x20);
                    }
                    else
                    {
                        guard.MoveY = obj.WorldY < candidateWorldY
                            ? (sbyte)8
                            : obj.WorldY > candidateWorldY
                                ? (sbyte)-8
                                : (sbyte)0;
                        obj.WorldX = (short)(door.TargetX + 0x20);
                    }

                    UpdateOctantFromMovement(
                        ref guard,
                        guard.MoveX,
                        guard.MoveY);

                    // FUN_700A forces FUN_6EE0 while the guard is still state 6,
                    // then changes the state byte to 0x11.
                    RefreshDirectionalSequence(
                        ref guard,
                        ref obj,
                        definition,
                        playerWorldX,
                        playerWorldY,
                        true);

                    guard.State = (byte)OriginalGuardState.RecoverMove11;
                    result.EnteredRecoverMove11 = true;
                }

                // FUN_188A is called for every activatable state-1 door and the
                // current movement attempt remains blocked this frame.
                result.DoorToggleRequested = true;
                result.Blocked = true;
                return result;
            }

            // Remaining wall-property bit 0x02 family is solid to movement.
            result.Blocked =
                (wallFlags & OriginalMapTables.WallAnyBlockingFamily) != 0;
            return result;
        }

        /// <summary>
        /// Recovered coordinate/collision core of FUN_1010_71DC.
        /// isBlockedAt models FUN_1010_700A (true = blocked). Map-cell occupancy
        /// bookkeeping and its side effects stay outside this helper.
        /// </summary>
        public static OriginalGuardMovementResult TickMovementCollisionCore(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj,
            Func<short, short, bool> isBlockedAt,
            ushort randomValue)
        {
            if (isBlockedAt == null)
                throw new ArgumentNullException(nameof(isBlockedAt));

            TickVerticalBob(ref guard, ref obj);

            sbyte originalMoveX = guard.MoveX;
            sbyte originalMoveY = guard.MoveY;

            short xProbe = (short)(
                obj.WorldX + originalMoveX + DirectionPadding(originalMoveX));
            short yProbe = (short)(
                obj.WorldY + originalMoveY + DirectionPadding(originalMoveY));

            bool xBlocked = originalMoveX != 0 &&
                (isBlockedAt(xProbe, (short)(obj.WorldY - 0x10)) ||
                 isBlockedAt(xProbe, (short)(obj.WorldY + 0x10)));

            bool yBlocked = originalMoveY != 0 &&
                (isBlockedAt((short)(obj.WorldX - 0x10), yProbe) ||
                 isBlockedAt((short)(obj.WorldX + 0x10), yProbe));

            short appliedX = xBlocked ? (short)0 : originalMoveX;
            short appliedY = yBlocked ? (short)0 : originalMoveY;

            // State 8 refuses the entire coordinate commit if either axis is
            // blocked. Other movement states commit the unblocked axis.
            bool positionCommitted =
                guard.State != (byte)OriginalGuardState.Move08 ||
                (!xBlocked && !yBlocked);

            if (positionCommitted)
            {
                obj.WorldX = (short)(obj.WorldX + appliedX);
                obj.WorldY = (short)(obj.WorldY + appliedY);
            }

            bool bounced = false;
            int octantX = appliedX;
            int octantY = appliedY;

            if (guard.State == (byte)OriginalGuardState.MoveThen03 &&
                xBlocked &&
                yBlocked)
            {
                bounced = true;

                // The Win16 assembly negates one byte component and writes only
                // the low byte into a zeroed 16-bit local before FUN_6E66.
                // Preserve that compiler-visible behavior here.
                if ((randomValue & 1) != 0)
                {
                    guard.MoveX = unchecked((sbyte)-guard.MoveX);
                    octantX = unchecked((byte)guard.MoveX);
                    octantY = 0;
                }
                else
                {
                    guard.MoveY = unchecked((sbyte)-guard.MoveY);
                    octantX = 0;
                    octantY = unchecked((byte)guard.MoveY);
                }
            }

            if (octantX != 0 || octantY != 0)
                AdvanceMovementFrame(ref guard, ref obj);

            UpdateOctantFromMovement(ref guard, octantX, octantY);

            return new OriginalGuardMovementResult
            {
                XBlocked = xBlocked,
                YBlocked = yBlocked,
                PositionCommitted = positionCommitted,
                Bounced = bounced,
                AppliedX = appliedX,
                AppliedY = appliedY
            };
        }

        public static OriginalGuardDispatchResult TickState06Movement(
            ref OriginalGuardRecord guard,
            ref OriginalObjectRecord obj,
            OriginalObjectDefinitionRecord definition,
            short playerWorldX,
            short playerWorldY,
            Func<short, short, bool> isBlockedAt,
            ushort randomValue,
            out OriginalGuardMovementResult movement)
        {
            movement = default;

            if (guard.State != (byte)OriginalGuardState.MoveThen03)
                return OriginalGuardDispatchResult.NotHandled;

            RefreshDirectionalSequence(
                ref guard,
                ref obj,
                definition,
                playerWorldX,
                playerWorldY,
                false);

            movement = TickMovementCollisionCore(
                ref guard,
                ref obj,
                isBlockedAt,
                randomValue);

            guard.Timer--;

            if (guard.Timer == 0)
            {
                guard.State = (byte)OriginalGuardState.Detection03;
                return OriginalGuardDispatchResult.Transitioned;
            }

            if (movement.AppliedX != 0 || movement.AppliedY != 0)
                return movement.PositionCommitted
                    ? OriginalGuardDispatchResult.Moved
                    : OriginalGuardDispatchResult.MovementBlocked;

            return movement.XBlocked || movement.YBlocked
                ? OriginalGuardDispatchResult.MovementBlocked
                : OriginalGuardDispatchResult.Waiting;
        }

        /// <summary>
        /// Recovered normal directional movement step. Eight facings collapse
        /// into four cardinal vectors. Strategy 2 doubles 8 world units to 16.
        /// </summary>
        public static void GetDirectionalStep(
            byte facing,
            byte strategy,
            out sbyte moveX,
            out sbyte moveY)
        {
            int scale = strategy == 2 ? 16 : 8;

            switch (facing & 7)
            {
                case 0:
                case 7:
                    moveX = 0;
                    moveY = (sbyte)-scale;
                    break;
                case 1:
                case 2:
                    moveX = (sbyte)scale;
                    moveY = 0;
                    break;
                case 3:
                case 4:
                    moveX = 0;
                    moveY = (sbyte)scale;
                    break;
                default:
                    moveX = (sbyte)-scale;
                    moveY = 0;
                    break;
            }
        }

        /// <summary>
        /// Confirmed state-0x07 decision slice after the original facing update.
        /// If the shared processing gate is set or perception fails, state 7 remains.
        /// Successful perception with strategy 3 enters state 0x13; otherwise state 2.
        /// </summary>
        public static OriginalGuardDispatchResult ResolveState07Perception(
            ref OriginalGuardRecord guard,
            bool processingGateSet,
            bool perceptionSucceeded,
            ushort randomValue)
        {
            if (guard.State != (byte)OriginalGuardState.Active07)
                return OriginalGuardDispatchResult.NotHandled;

            if (processingGateSet || !perceptionSucceeded)
                return OriginalGuardDispatchResult.Waiting;

            if (guard.Strategy == 3)
            {
                EnterStrategy3TimedMove(ref guard, randomValue);
                return OriginalGuardDispatchResult.Transitioned;
            }

            guard.State = (byte)OriginalGuardState.Active02;
            return OriginalGuardDispatchResult.Transitioned;
        }

        // State 0x06: once its movement/timer work completes, continue at 0x03.
        public static OriginalGuardDispatchResult CompleteState06(
            ref OriginalGuardRecord guard)
        {
            if (guard.State != (byte)OriginalGuardState.MoveThen03)
                return OriginalGuardDispatchResult.NotHandled;

            guard.State = (byte)OriginalGuardState.Detection03;
            return OriginalGuardDispatchResult.Completed;
        }

        // State 0x11: after the confirmed movement/timer sequence, strategy is
        // cleared and normal state-7 processing resumes.
        public static OriginalGuardDispatchResult CompleteState11(
            ref OriginalGuardRecord guard)
        {
            if (guard.State != (byte)OriginalGuardState.RecoverMove11)
                return OriginalGuardDispatchResult.NotHandled;

            guard.Strategy = 0;
            guard.State = (byte)OriginalGuardState.Active07;
            return OriginalGuardDispatchResult.Completed;
        }

        // States 0x0A/0x0B share a no-local-action dispatcher target.
        public static bool IsNoLocalActionState(byte state)
        {
            return state == (byte)OriginalGuardState.NoLocalAction0A ||
                   state == (byte)OriginalGuardState.NoLocalAction0B;
        }
    }
}
