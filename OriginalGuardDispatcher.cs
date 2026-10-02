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

        // State 0x00 and 0x12 both return through nextstate after their
        // sequence/timer completion. The exact frame producer stays external.
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

        // State 0x15: confirmed damage reaction return.
        // The original advances the selected reaction sequence first; callers invoke
        // this only when that sequence reaches its final frame.
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
