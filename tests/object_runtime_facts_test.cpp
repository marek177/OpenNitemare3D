#include "game/ObjectSystem.hpp"

#include <cassert>
#include <cstddef>

int main() {
    using namespace nitemare3d::game;

    static_assert(sizeof(ObjectRuntimeRecord) == 0x1C);
    static_assert(kObjectRecordSize == 0x1C);
    static_assert(offsetof(ObjectRuntimeRecord, objectId) == 0x00);
    static_assert(offsetof(ObjectRuntimeRecord, variant) == 0x01);
    static_assert(offsetof(ObjectRuntimeRecord, animationFrame) == 0x03);
    static_assert(offsetof(ObjectRuntimeRecord, sequenceId) == 0x04);
    static_assert(offsetof(ObjectRuntimeRecord, animationDeadline) == 0x08);
    static_assert(offsetof(ObjectRuntimeRecord, flags) == 0x05);
    static_assert(offsetof(ObjectRuntimeRecord, type) == 0x06);
    static_assert(offsetof(ObjectRuntimeRecord, guardIndex) == 0x07);
    static_assert(offsetof(ObjectRuntimeRecord, mapCellOffset) == 0x0C);
    static_assert(offsetof(ObjectRuntimeRecord, mapCellSegment) == 0x0E);
    static_assert(offsetof(ObjectRuntimeRecord, worldX) == 0x10);
    static_assert(offsetof(ObjectRuntimeRecord, worldY) == 0x12);
    static_assert(offsetof(ObjectRuntimeRecord, renderSortA) == 0x14);
    static_assert(offsetof(ObjectRuntimeRecord, renderSortB) == 0x16);
    static_assert(offsetof(ObjectRuntimeRecord, projectedYBase) == 0x18);
    static_assert(offsetof(ObjectRuntimeRecord, runtime1A) == 0x1A);

    static_assert(kObjectRuntimePresent == 0x01);
    static_assert(kObjectBlocksMovement == 0x02);
    static_assert(kObjectSpecialTouch == 0x04);
    static_assert(kObjectCreatesGuard == 0x08);
    static_assert(kObjectSpecial20 == 0x20);
    static_assert(kObjectSpecial40 == 0x40);
    static_assert(kObjectProjectedDamageBaselineOffset == 0x18);

    // Basic sanity check that the verified movement/guard flags do not overlap.
    assert((kObjectBlocksMovement & kObjectCreatesGuard) == 0);
}
