namespace Nitemare3D
{
    /// <summary>
    /// Clean-room facts recovered from the original NITE3W.EXE V1.10 GUARD
    /// runtime. This is deliberately separate from Guard.cs: the current C#
    /// Guard state machine is a reimplementation and is not yet a 1:1 copy of
    /// the original 22-state dispatcher.
    /// </summary>
    public static class RecoveredGuardRuntime
    {
        public const int RecordSize = 0x1A;
        public const int Capacity = 100;
        public const byte FreshStrength = 0xFF;
        public const int StateCount = 0x16;

        // Original GUARD record offsets.
        public const int TimeStampOffset = 0x02;
        public const int TimerOffset = 0x06;
        public const int ObjectSlotOffset = 0x08;
        public const int StrategyOffset = 0x0A;
        public const int StateOffset = 0x0B;
        public const int NextStateOffset = 0x0C;
        public const int ObjectIdOffset = 0x0D;
        public const int DefinitionIdOffset = 0x0E; // Semantic label still PARTIAL.
        public const int SyncFlagOffset = 0x0F;     // Semantic label still PARTIAL.
        public const int StrengthOffset = 0x10;
        public const int OctantOffset = 0x11;
        public const int ResultOctantOffset = 0x12;
        public const int TransitionParamOffset = 0x13; // PARTIAL.
        public const int TransitionFlagOffset = 0x16;  // PARTIAL.

        // The original executable dispatches numeric states 0x00..0x15. Only
        // 0x15 has a final high-level name strong enough to promote here.
        public enum State : byte
        {
            State00 = 0x00,
            State01 = 0x01,
            State02 = 0x02,
            State03 = 0x03,
            State04 = 0x04,
            State05 = 0x05,
            State06 = 0x06,
            State07 = 0x07,
            State08 = 0x08,
            State09 = 0x09,
            State0A = 0x0A,
            State0B = 0x0B,
            State0C = 0x0C,
            State0D = 0x0D,
            State0E = 0x0E,
            State0F = 0x0F,
            State10 = 0x10,
            State11 = 0x11,
            State12 = 0x12,
            State13 = 0x13,
            State14 = 0x14,
            PainReaction = 0x15,
        }

        // Segment-3 handler offsets from the original Win16 state dispatcher.
        // These are reference anchors for future 1:1 porting, not function
        // pointers usable by the managed reimplementation.
        public static readonly ushort[] StateHandlerOffsets =
        {
            0x7BA2, // 00 animation/timer -> next state
            0x7BE0, // 01 timer -> 02
            0x7BFA, // 02 active AI/animation + sound path
            0x7C3C, // 03 detection/transition-like
            0x7C86, // 04 alternate detection/attack-like
            0x7CE4, // 05 helper transition
            0x7CEC, // 06 movement + timer -> 03
            0x7D2A, // 07 active AI; strategy 3 special
            0x7D7E, // 08 movement/AI; may -> 02
            0x7DEC, // 09 special/collision action
            0x80A4, // 0A no local action
            0x80A4, // 0B no local action
            0x7E54, // 0C shared
            0x7E54, // 0D shared
            0x7E6C, // 0E conditional -> 0F
            0x7E9E, // 0F timer/action -> 10 or 0E
            0x7F26, // 10 timer -> 0F
            0x7F8E, // 11 movement + timer -> strategy=0,state=07
            0x7FEE, // 12 wait -> next state
            0x8038, // 13 helper transition
            0x804A, // 14 long timer + periodic action
            0x807E, // 15 pain/hit -> next state
        };

        // OBJECT+06 classes 0x08..0x20 are the original score-switch inputs
        // for GUARD1..GUARD25. Anything outside the switch returns zero.
        public const byte FirstScoredObjectClass = 0x08;
        public const byte LastScoredObjectClass = 0x20;

        private static readonly int[] ScoreByObjectClass =
        {
            25,    // 0x08 GUARD1  Bat
            75,    // 0x09 GUARD2  Frankenstein
            50,    // 0x0A GUARD3  Mummy
            100,   // 0x0B GUARD4  Skeleton
            250,   // 0x0C GUARD5  Mrs H.
            150,   // 0x0D GUARD6  Zelda
            200,   // 0x0E GUARD7  Vampira
            100,   // 0x0F GUARD8  Baddie #1
            100,   // 0x10 GUARD9  Baddie #2
            0,     // 0x11 GUARD10 Dracula
            150,   // 0x12 GUARD11 Cemetery Gargoyle
            150,   // 0x13 GUARD12 Garden Gargoyle
            200,   // 0x14 GUARD13 unknown/unused identity
            -1000, // 0x15 GUARD14 Penelope
            1000,  // 0x16 GUARD15 Dr. Hamerstein
            100,   // 0x17 GUARD16 Tall slim robot
            200,   // 0x18 GUARD17 Trashcan robot
            0,     // 0x19 GUARD18 Cannon
            25,    // 0x1A GUARD19 Ghost
            100,   // 0x1B GUARD20 Goldie
            100,   // 0x1C GUARD21 Greenie
            250,   // 0x1D GUARD22 Demon
            250,   // 0x1E GUARD23 Alien #1
            200,   // 0x1F GUARD24 Alien #2
            50,    // 0x20 GUARD25 unknown/unused identity
        };

        public static int ScoreForObjectClass(byte objectClass)
        {
            if (objectClass < FirstScoredObjectClass ||
                objectClass > LastScoredObjectClass)
            {
                return 0;
            }

            return ScoreByObjectClass[objectClass - FirstScoredObjectClass];
        }

        /// <summary>
        /// Final player->GUARD difficulty transform recovered from the original
        /// executable. The input here is the class/weapon-transformed damage.
        /// </summary>
        public static int ApplyPlayerDamageDifficulty(int damage, ushort difficulty)
        {
            switch (difficulty)
            {
                case RecoveredRuntime.DifficultyEasier:
                    return damage * 2;
                case RecoveredRuntime.DifficultyHarder:
                    return damage / 2;
                default:
                    return damage;
            }
        }

        /// <summary>
        /// Final GUARD/object->player difficulty transform recovered from the
        /// original executable. The class/distance transform happens earlier.
        /// </summary>
        public static int ApplyEnemyDamageDifficulty(int damage, ushort difficulty)
        {
            switch (difficulty)
            {
                case RecoveredRuntime.DifficultyEasier:
                    return damage / 2;
                case RecoveredRuntime.DifficultyHarder:
                    return damage * 2;
                default:
                    return damage;
            }
        }
    }
}
