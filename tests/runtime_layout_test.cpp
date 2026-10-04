#include "game/GuardSystem.hpp"
#include "game/ObjectSystem.hpp"

#include <cassert>
#include <cstddef>

int main() {
    using namespace nitemare3d::game;

    static_assert(sizeof(ObjectRuntimeRecord) == 0x1C);
    static_assert(offsetof(ObjectRuntimeRecord, flags) == 0x05);
    static_assert(offsetof(ObjectRuntimeRecord, type) == 0x06);
    static_assert(offsetof(ObjectRuntimeRecord, guardIndex) == 0x07);
    static_assert(offsetof(ObjectRuntimeRecord, worldX) == 0x10);
    static_assert(offsetof(ObjectRuntimeRecord, worldY) == 0x12);
    static_assert(offsetof(ObjectRuntimeRecord, projectedYBase) == 0x18);

    static_assert(sizeof(GuardRuntimeRecord) == 0x1A);
    static_assert(offsetof(GuardRuntimeRecord, stateTimer) == 0x06);
    static_assert(offsetof(GuardRuntimeRecord, objectSlot) == 0x08);
    static_assert(offsetof(GuardRuntimeRecord, strategy) == 0x0A);
    static_assert(offsetof(GuardRuntimeRecord, state) == 0x0B);
    static_assert(offsetof(GuardRuntimeRecord, nextState) == 0x0C);
    static_assert(offsetof(GuardRuntimeRecord, strength) == 0x10);

    assert(guardScoreForObjectClass(0x08) == 25);
    assert(guardScoreForObjectClass(0x16) == 1000);
    assert(guardScoreForObjectClass(0x15) == -1000);

    return 0;
}
