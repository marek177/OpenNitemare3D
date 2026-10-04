#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace nitemare3d::game {

inline constexpr std::size_t kObjectRecordSize = 0x1C; // VERIFIED_EXE

// Clean-room partial layout of the original NITE3W V1.10 runtime object record.
// Only fields with direct executable evidence are named. PARTIAL fields are
// intentionally named by observed role rather than guessed original symbols.
#pragma pack(push, 1)
struct ObjectRuntimeRecord {
    std::uint8_t objectId;        // +0x00, object/map identifier (VERIFIED_EXE)
    std::uint8_t variant;         // +0x01, class-relative variant index (VERIFIED_EXE); gameplay meaning PARTIAL
    std::uint8_t animationAux;    // +0x02, animation/render auxiliary byte; full semantics PARTIAL
    std::uint8_t animationFrame;  // +0x03, current sequence frame (VERIFIED_EXE)
    std::uint8_t sequenceId;      // +0x04, runtime sequence index (VERIFIED_EXE)
    std::uint8_t flags;           // +0x05, runtime/property flags (VERIFIED_EXE)
    std::uint8_t type;            // +0x06, runtime class lookup from definition (VERIFIED_EXE)
    std::uint8_t guardIndex;      // +0x07, assigned from current GUARD count for guard objects
    std::uint32_t animationDeadline; // +0x08, absolute sequence/frame deadline (VERIFIED_EXE)
    std::uint16_t mapCellOffset;  // +0x0C, near part of persisted map-cell far pointer
    std::uint16_t mapCellSegment; // +0x0E, segment part of map-cell far pointer
    std::int16_t worldX;          // +0x10, world-space X coordinate (VERIFIED_EXE)
    std::int16_t worldY;          // +0x12, world-space Y coordinate (VERIFIED_EXE)
    std::int16_t renderSortA;     // +0x14, renderer/sort-related value (PARTIAL)
    std::int16_t renderSortB;     // +0x16, renderer/sort-related value (PARTIAL)
    std::int16_t projectedYBase;  // +0x18, FUN_1010_CC7C projection/depth-scale cache read by damage producer; not world Y
    std::uint8_t runtime1A;       // +0x1A, zero for ordinary objects; embedded projectile OBJECT uses a vertical sprite offset (5..20 in flight)
    std::uint8_t unknown1B;
};
#pragma pack(pop)

static_assert(sizeof(ObjectRuntimeRecord) == kObjectRecordSize);
static_assert(offsetof(ObjectRuntimeRecord, animationFrame) == 0x03);
static_assert(offsetof(ObjectRuntimeRecord, sequenceId) == 0x04);
static_assert(offsetof(ObjectRuntimeRecord, flags) == 0x05);
static_assert(offsetof(ObjectRuntimeRecord, type) == 0x06);
static_assert(offsetof(ObjectRuntimeRecord, guardIndex) == 0x07);
static_assert(offsetof(ObjectRuntimeRecord, animationDeadline) == 0x08);
static_assert(offsetof(ObjectRuntimeRecord, mapCellOffset) == 0x0C);
static_assert(offsetof(ObjectRuntimeRecord, mapCellSegment) == 0x0E);
static_assert(offsetof(ObjectRuntimeRecord, worldX) == 0x10);
static_assert(offsetof(ObjectRuntimeRecord, worldY) == 0x12);
static_assert(offsetof(ObjectRuntimeRecord, renderSortA) == 0x14);
static_assert(offsetof(ObjectRuntimeRecord, renderSortB) == 0x16);
static_assert(offsetof(ObjectRuntimeRecord, projectedYBase) == 0x18);
static_assert(offsetof(ObjectRuntimeRecord, runtime1A) == 0x1A);

inline constexpr std::uint8_t kObjectRuntimePresent = 0x01; // 0x7F94: instantiate runtime OBJECT
inline constexpr std::uint8_t kObjectBlocksMovement = 0x02; // 0x7F94: blocks player movement
inline constexpr std::uint8_t kObjectSpecialTouch = 0x04;   // 0x7F94: special/touch handler; exact subtype semantics PARTIAL
inline constexpr std::uint8_t kObjectCreatesGuard = 0x08;   // 0x7F94 / OBJECT+05: create GUARD
inline constexpr std::uint8_t kObjectSpecial20 = 0x20;      // exact semantic TODO
inline constexpr std::uint8_t kObjectSpecial40 = 0x40;      // exact semantic TODO



constexpr bool objectRuntimePresent(std::uint8_t flags) noexcept {
    return (flags & kObjectRuntimePresent) != 0;
}

constexpr bool objectBlocksMovement(std::uint8_t flags) noexcept {
    return (flags & kObjectBlocksMovement) != 0;
}

constexpr bool objectHasSpecialTouch(std::uint8_t flags) noexcept {
    return (flags & kObjectSpecialTouch) != 0;
}

constexpr bool objectCreatesGuard(std::uint8_t flags) noexcept {
    return (flags & kObjectCreatesGuard) != 0;
}

// Keep the two unresolved high bits queryable without assigning unsupported
// semantic names. They are still useful when comparing definition/runtime data.
constexpr bool objectHasSpecial20(std::uint8_t flags) noexcept {
    return (flags & kObjectSpecial20) != 0;
}

constexpr bool objectHasSpecial40(std::uint8_t flags) noexcept {
    return (flags & kObjectSpecial40) != 0;
}

// Damage path cross-binding (see docs/COMBAT_DAMAGE_RE.md): OBJECT+0x18 is read
// by seg3:9FA2 as a projected/view-space vertical value. It must not be confused
// with worldY at +0x12. The exact writer-level renderer symbol remains open.
inline constexpr std::size_t kObjectProjectedDamageBaselineOffset = 0x18;

// v0.13 / 2026-09-26 Win16 v1.8 pushable audit.
// OBJECT type 0x28 is represented by a separate six-byte PUSH runtime record.
inline constexpr std::uint8_t kPushableObjectType = 0x28;
inline constexpr std::size_t kPushRecordSize = 0x06;
inline constexpr std::size_t kPushCapacity = 12;
inline constexpr std::uint8_t kPushMoveTicks = 8;
inline constexpr std::int8_t kPushUnitsPerTick = 8;
inline constexpr int kWorldUnitsPerTile = 64;

#pragma pack(push, 1)
struct PushRuntimeRecord {
    std::uint16_t objectIndex;     // +00 OBJECT slot
    std::int8_t deltaX;            // +02 signed world-X step per tick
    std::int8_t deltaY;            // +03 signed world-Y step per tick
    std::uint8_t stepsRemaining;   // +04 0 when idle; 8 when a one-tile push starts
    std::uint8_t runtime05;        // +05 unresolved auxiliary/runtime byte
};
#pragma pack(pop)

static_assert(sizeof(PushRuntimeRecord) == kPushRecordSize);
static_assert(offsetof(PushRuntimeRecord, objectIndex) == 0x00);
static_assert(offsetof(PushRuntimeRecord, deltaX) == 0x02);
static_assert(offsetof(PushRuntimeRecord, deltaY) == 0x03);
static_assert(offsetof(PushRuntimeRecord, stepsRemaining) == 0x04);
static_assert(offsetof(PushRuntimeRecord, runtime05) == 0x05);

struct PushDirection {
    std::int8_t dx;
    std::int8_t dy;
};

// Player octant (angle / 45) is intentionally eight-way, while push movement
// collapses pairs of octants to four cardinal directions.
inline constexpr std::array<std::int16_t, 8> kFrontCellDelta = {
    -64, 1, 1, 64, 64, -1, -1, -64
};
inline constexpr std::array<std::int8_t, 8> kPushDeltaX = {
    0, 8, 8, 0, 0, -8, -8, 0
};
inline constexpr std::array<std::int8_t, 8> kPushDeltaY = {
    -8, 0, 0, 8, 8, 0, 0, -8
};

constexpr PushDirection pushDirectionForOctant(std::uint8_t octant) noexcept {
    const auto i = static_cast<std::size_t>(octant & 7u);
    return {kPushDeltaX[i], kPushDeltaY[i]};
}

constexpr bool canStartPush(const PushRuntimeRecord& push,
                            std::uint8_t targetSemanticFlags) noexcept {
    return push.stepsRemaining == 0 && (targetSemanticFlags & 0x02u) == 0;
}

constexpr int pushDistanceForCompletedMove() noexcept {
    return static_cast<int>(kPushMoveTicks) * static_cast<int>(kPushUnitsPerTick);
}

static_assert(pushDistanceForCompletedMove() == kWorldUnitsPerTile);

} // namespace nitemare3d::game
