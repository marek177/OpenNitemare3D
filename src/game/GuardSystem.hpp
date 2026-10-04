#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace nitemare3d::game {

inline constexpr std::size_t kGuardCapacity = 100;
inline constexpr std::size_t kGuardRecordSize = 0x1A;
inline constexpr std::size_t kGuardSaveOffset = 0xB43B;
inline constexpr std::size_t kGuardSaveSize = 0x0A28;
inline constexpr std::uint8_t kFreshGuardStrength = 0xFF;

// Clean-room runtime layout recovered from NITE3W.EXE V1.10.
// Names below are promoted only where direct executable evidence exists.
#pragma pack(push, 1)
struct GuardRuntimeRecord {
    std::uint8_t unknown00_01[0x02];
    std::uint32_t timeStamp;          // +02 VERIFIED_EXE
    std::uint16_t stateTimer;         // +06 VERIFIED_EXE; debug label "timer"
    std::uint16_t objectSlot;         // +08 VERIFIED_EXE; index * 0x1C -> OBJECT
    std::uint8_t strategy;            // +0A VERIFIED_EXE; debug label "strategy"
    std::uint8_t state;               // +0B VERIFIED_EXE
    std::uint8_t nextState;           // +0C VERIFIED_EXE
    std::uint8_t savedMapObjectByte; // +0D: displaced map-cell object byte used on movement/death
    std::uint8_t definitionId;        // +0E PARTIAL: lookup-derived
    std::uint8_t syncFlag;            // +0F PARTIAL: derived boolean
    std::uint8_t strength;            // +10 VERIFIED_EXE: strength / HP
    std::uint8_t octant;              // +11 VERIFIED_EXE; debug label "octant"
    std::uint8_t resultOctant;        // +12 VERIFIED_EXE; debug label "resoct"
    std::int8_t moveX;                 // +13 VERIFIED_EXE in state 0x13
    std::int8_t moveY;                 // +14 VERIFIED_EXE in state 0x13
    std::uint8_t unknown15;            // +15 semantic TODO
    std::uint8_t transitionFlag;       // +16 PARTIAL control flag
    std::uint8_t unknown17_19[0x03];
};
#pragma pack(pop)

static_assert(sizeof(GuardRuntimeRecord) == kGuardRecordSize);
static_assert(offsetof(GuardRuntimeRecord, timeStamp) == 0x02);
static_assert(offsetof(GuardRuntimeRecord, stateTimer) == 0x06);
static_assert(offsetof(GuardRuntimeRecord, objectSlot) == 0x08);
static_assert(offsetof(GuardRuntimeRecord, strategy) == 0x0A);
static_assert(offsetof(GuardRuntimeRecord, state) == 0x0B);
static_assert(offsetof(GuardRuntimeRecord, nextState) == 0x0C);
static_assert(offsetof(GuardRuntimeRecord, savedMapObjectByte) == 0x0D);
static_assert(offsetof(GuardRuntimeRecord, definitionId) == 0x0E);
static_assert(offsetof(GuardRuntimeRecord, syncFlag) == 0x0F);
static_assert(offsetof(GuardRuntimeRecord, strength) == 0x10);
static_assert(offsetof(GuardRuntimeRecord, octant) == 0x11);
static_assert(offsetof(GuardRuntimeRecord, resultOctant) == 0x12);
static_assert(offsetof(GuardRuntimeRecord, moveX) == 0x13);
static_assert(offsetof(GuardRuntimeRecord, moveY) == 0x14);
static_assert(offsetof(GuardRuntimeRecord, unknown15) == 0x15);
static_assert(offsetof(GuardRuntimeRecord, transitionFlag) == 0x16);
static_assert(kGuardCapacity * sizeof(GuardRuntimeRecord) == kGuardSaveSize);

// The dispatcher accepts exactly 0x00..0x15. Only state 0x15 has a final
// high-level name established strongly enough to encode here. The other names
// remain numeric until their animation/sound/movement semantics are complete.
enum class GuardState : std::uint8_t {
    State00 = 0x00,
    State01 = 0x01,
    State02 = 0x02,
    State03 = 0x03,
    State04 = 0x04,
    State05 = 0x05,
    State06 = 0x06,
    PerceptionDecision = 0x07,
    State07 = PerceptionDecision,
    State08 = 0x08,
    State09 = 0x09,
    State0A = 0x0A,
    LethalPlayerContact = 0x0B,
    State0B = LethalPlayerContact,
    State0C = 0x0C,
    State0D = 0x0D,
    State0E = 0x0E,
    State0F = 0x0F,
    State10 = 0x10,
    State11 = 0x11,
    State12 = 0x12,
    TimedDirectionalMove = 0x13,
    State13 = TimedDirectionalMove,
    State14 = 0x14,
    PainReaction = 0x15,
};

inline constexpr std::size_t kGuardStateCount = 0x16;

// 2026-09-24 Win16 audit: 0x0B is written only on lethal guard-to-player
// contact and has no dispatcher case. State 0x13 is the strategy-3 move path.
inline constexpr std::uint8_t kGuardStateLethalPlayerContact = 0x0B;
inline constexpr std::uint8_t kGuardStateStrategy3Movement = 0x13;
inline constexpr std::uint16_t kGuardState13TimerRandomRange = 0x50;
inline constexpr std::uint16_t kGuardState13TimerMinimum = 8;

struct GuardMoveVector {
    std::int8_t dx;
    std::int8_t dy;
};

// FUN_1010_6F2A (Win16 v1.8): facing 0..7 is cardinalized in pairs.
// Normal movement is 8 world units; strategy 2 doubles it to 16.
constexpr GuardMoveVector guardDirectionalStep(std::uint8_t facing,
                                               std::uint8_t strategy) noexcept {
    constexpr std::array<std::int8_t, 8> dx = {0, 1, 1, 0, 0, -1, -1, 0};
    constexpr std::array<std::int8_t, 8> dy = {-1, 0, 0, 1, 1, 0, 0, -1};
    const auto i = static_cast<std::size_t>(facing & 7u);
    const std::int8_t scale = strategy == 2 ? 16 : 8;
    return {
        static_cast<std::int8_t>(dx[i] * scale),
        static_cast<std::int8_t>(dy[i] * scale)
    };
}

struct GuardInitialProfile {
    std::uint8_t strategy;
    std::uint8_t state;
    std::uint8_t nextState;
    std::uint8_t perceptionMode;
};

// FUN_1010_AF7E class-specific initialization. This captures only assignments
// backed by the 2026-09-26 static audit; later movement may promote state to 8.
constexpr GuardInitialProfile guardInitialProfile(std::uint8_t objectClass) noexcept {
    GuardInitialProfile p{0, 7, 2, 1};

    switch (objectClass) {
    case 0x08:
    case 0x09:
    case 0x0A:
    case 0x11:
    case 0x14:
    case 0x1A:
        p.perceptionMode = 0;
        break;
    case 0x12:
    case 0x13:
        p.strategy = 3;
        p.perceptionMode = 0;
        break;
    case 0x15:
    case 0x16:
        p.nextState = 0;
        break;
    case 0x19:
        p.strategy = 4;
        p.state = 0x0E;
        break;
    case 0x21:
        p.state = 0;
        p.nextState = 0;
        break;
    default:
        break;
    }
    return p;
}

constexpr std::uint16_t guardState13InitialTimer(std::uint16_t randomValue) noexcept {
    return static_cast<std::uint16_t>(
        randomValue % kGuardState13TimerRandomRange + kGuardState13TimerMinimum);
}

// Control-flow summary recovered from the state dispatcher. These are handler
// offsets inside the original segment-3 code, useful for cross-checking IDA /
// Ghidra without pretending that all state names are already known.
inline constexpr std::array<std::uint16_t, kGuardStateCount> kGuardStateHandlerOffsets = {
    0x7BA2, // 00 animation/timer -> nextState
    0x7BE0, // 01 timer -> 02
    0x7BFA, // 02 active AI/animation + sound path
    0x7C3C, // 03 detection/transition-like
    0x7C86, // 04 alternate detection/attack-like
    0x7CE4, // 05 helper transition
    0x7CEC, // 06 movement + timer -> 03
    0x7D2A, // 07 active AI; strategy 3 special branch
    0x7D7E, // 08 movement/AI; may -> 02
    0x7DEC, // 09 special/collision action
    0x80A4, // 0A no local action in dispatcher
    0x80A4, // 0B no local action in dispatcher
    0x7E54, // 0C shared handler
    0x7E54, // 0D shared handler
    0x7E6C, // 0E conditional -> 0F
    0x7E9E, // 0F timer/action -> 10 or 0E
    0x7F26, // 10 timer -> 0F
    0x7F8E, // 11 movement + timer -> strategy=0,state=07
    0x7FEE, // 12 wait -> nextState
    0x8038, // 13 helper transition
    0x804A, // 14 long timer + periodic action
    0x807E, // 15 confirmed pain/hit -> nextState
};

// State 0x13 countdown from the 2026-09-24 Win16 audit. The timer always
// decreases, including when the attempted map step is blocked. A step commits
// coordinates/cell occupancy only when targetCellAllowsMove is true.
struct GuardState13StepResult {
    std::uint16_t nextTimer{};
    bool playMovementSound{};
    bool attemptMovement{};
    bool commitMovement{};
    bool clearStrategyAndEnterState2{};
};

constexpr GuardState13StepResult stepGuardState13(
    std::uint16_t currentTimer,
    bool targetCellAllowsMove) noexcept {
    if (currentTimer == 0) {
        return {0, false, false, false, true};
    }

    const std::uint16_t nextTimer =
        static_cast<std::uint16_t>(currentTimer - std::uint16_t{1});
    if (nextTimer == 8) {
        return {nextTimer, true, false, false, false};
    }
    if (nextTimer < 8) {
        return {nextTimer, false, true, targetCellAllowsMove, false};
    }
    return {nextTimer, false, false, false, false};
}

// Original score dispatcher: OBJECT+06 class values 0x08..0x20 map to
// GUARD1..GUARD25. Classes outside that switch return zero. GUARD26/Dancers is
// therefore on the default zero-score path rather than having an entry here.
inline constexpr std::uint8_t kFirstScoredGuardObjectClass = 0x08;
inline constexpr std::uint8_t kLastScoredGuardObjectClass = 0x20;
inline constexpr std::array<int, 25> kGuardScoreByObjectClass = {
    25,    // 0x08 GUARD1  Bat
    75,    // 0x09 GUARD2  Frankenstein
    50,    // 0x0A GUARD3  Mummy
    100,   // 0x0B GUARD4  Skeleton
    250,   // 0x0C GUARD5  Mrs H.
    150,   // 0x0D GUARD6  Zelda
    200,   // 0x0E GUARD7  Vampira
    100,   // 0x0F GUARD8  Baddie #1
    100,   // 0x10 GUARD9  Baddie #2
    0,     // 0x11 GUARD10 Dracula -- scripted behavior handled separately
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

constexpr int guardScoreForObjectClass(std::uint8_t objectClass) noexcept {
    if (objectClass < kFirstScoredGuardObjectClass ||
        objectClass > kLastScoredGuardObjectClass) {
        return 0;
    }
    return kGuardScoreByObjectClass[
        static_cast<std::size_t>(objectClass - kFirstScoredGuardObjectClass)];
}

static_assert(guardScoreForObjectClass(0x11) == 0);   // Dracula
static_assert(guardScoreForObjectClass(0x16) == 1000); // Dr. Hamerstein
static_assert(guardScoreForObjectClass(0x1D) == 250);  // Demon
static_assert(guardScoreForObjectClass(0x00) == 0);    // default path



enum class GuardHitDisposition : std::uint8_t {
    Ignored,
    Pain,
    Lethal,
};

struct GuardHitOutcome {
    std::uint8_t strength;
    std::uint8_t resultOctant;
    GuardHitDisposition disposition;
};

// Verified receiver contract from seg3:80F7/811C..81EF.
// Positive non-lethal hits subtract HP and write resoct=8.
// Lethal hits clear HP and branch to death/special handling.
// This helper intentionally does not guess the class-specific death handler.
constexpr GuardHitOutcome receiveGuardDamage(std::uint8_t strength,
                                             int damage,
                                             std::uint8_t currentResultOctant) noexcept {
    if (damage <= 0)
        return {strength, currentResultOctant, GuardHitDisposition::Ignored};
    if (damage >= strength)
        return {0, currentResultOctant, GuardHitDisposition::Lethal};
    return {
        static_cast<std::uint8_t>(strength - damage),
        8,
        GuardHitDisposition::Pain
    };
}

struct DraculaPhase2Reset {
    std::uint8_t objectClass;
    std::uint8_t strength;
    std::uint8_t state;
    std::uint8_t nextState;
    std::uint16_t timer;
    std::uint8_t sequenceValue;
    std::uint8_t eventId;
};

inline constexpr DraculaPhase2Reset kDraculaPhase2Reset{
    0x14, 0xFF, 0x08, 0x02, 1, 0x23, 0x22
};

// Damage receiver is structurally verified: a computed damage value is
// compared with strength. Lethal damage clears strength to zero and enters the
// death path. Positive non-lethal damage is subtracted, resultOctant is set to
// 8, and ordinary pain handling temporarily enters state 0x15 before restoring
// nextState. Player-weapon/class damage transforms are documented in
// docs/COMBAT_DAMAGE_RE.md; they are no longer an unknown producer.

} // namespace nitemare3d::game
