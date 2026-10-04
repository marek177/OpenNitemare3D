#include "game/GuardSystem.hpp"

#include <cassert>
#include <cstddef>

int main() {
    using namespace nitemare3d::game;

    static_assert(sizeof(GuardRuntimeRecord) == 26);
    static_assert(kGuardCapacity == 100);
    static_assert(kGuardSaveSize == 2600);
    static_assert(offsetof(GuardRuntimeRecord, stateTimer) == 0x06);
    static_assert(offsetof(GuardRuntimeRecord, objectSlot) == 0x08);
    static_assert(offsetof(GuardRuntimeRecord, strategy) == 0x0A);
    static_assert(offsetof(GuardRuntimeRecord, state) == 0x0B);
    static_assert(offsetof(GuardRuntimeRecord, nextState) == 0x0C);
    static_assert(offsetof(GuardRuntimeRecord, strength) == 0x10);
    static_assert(offsetof(GuardRuntimeRecord, octant) == 0x11);
    static_assert(offsetof(GuardRuntimeRecord, resultOctant) == 0x12);
    static_assert(kFreshGuardStrength == 0xFF);
    static_assert(kGuardStateCount == 22);
    static_assert(static_cast<unsigned>(GuardState::PainReaction) == 0x15);

    // Original score switch anchors.
    assert(guardScoreForObjectClass(0x08) == 25);    // Bat
    assert(guardScoreForObjectClass(0x11) == 0);     // Dracula
    assert(guardScoreForObjectClass(0x15) == -1000); // Penelope
    assert(guardScoreForObjectClass(0x16) == 1000);  // Dr. Hamerstein
    assert(guardScoreForObjectClass(0x1B) == 100);   // Goldie
    assert(guardScoreForObjectClass(0x1C) == 100);   // Greenie
    assert(guardScoreForObjectClass(0x1D) == 250);   // Demon
    assert(guardScoreForObjectClass(0x20) == 50);    // GUARD25 unknown

    // The audited state 0x13 sound tick, movement attempts and blocked-step
    // timer behavior are represented separately.
    static_assert(kGuardStateLethalPlayerContact == 0x0B);
    static_assert(kGuardStateStrategy3Movement == 0x13);
    assert(guardState13InitialTimer(0) == 8);
    assert(guardState13InitialTimer(79) == 87);
    assert(guardState13InitialTimer(80) == 8);
    const auto wait = stepGuardState13(10, true);
    assert(wait.nextTimer == 9 && !wait.playMovementSound && !wait.attemptMovement);
    const auto sound = stepGuardState13(9, true);
    assert(sound.nextTimer == 8 && sound.playMovementSound && !sound.attemptMovement);
    const auto move = stepGuardState13(8, true);
    assert(move.nextTimer == 7 && move.attemptMovement && move.commitMovement);
    const auto blocked = stepGuardState13(8, false);
    assert(blocked.nextTimer == 7 && blocked.attemptMovement && !blocked.commitMovement);
    const auto lastAttempt = stepGuardState13(1, false);
    assert(lastAttempt.nextTimer == 0 && lastAttempt.attemptMovement && !lastAttempt.commitMovement);
    std::uint16_t timer = 8;
    int blockedMovementAttempts = 0;
    for (int i = 0; i < 8; ++i) {
        const auto step = stepGuardState13(timer, false);
        assert(step.attemptMovement && !step.commitMovement);
        timer = step.nextTimer;
        ++blockedMovementAttempts;
    }
    assert(blockedMovementAttempts == 8 && timer == 0);

    const auto finished = stepGuardState13(timer, true);
    assert(finished.clearStrategyAndEnterState2 && !finished.attemptMovement && finished.nextTimer == 0);

    // Classes outside GUARD1..25 use the default score path.
    assert(guardScoreForObjectClass(0x07) == 0);
    assert(guardScoreForObjectClass(0x21) == 0); // includes GUARD26/Dancers path
}
