#pragma once

#include "game/ObjectSystem.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace nitemare3d::game {

// NITE3W 1.10 player-fired projectile pool. These are reference-layout facts,
// not a claim that the original far pointers can be reused in a modern process.
inline constexpr std::size_t kProjectilePoolCapacity = 8;
inline constexpr std::size_t kProjectileRecordStride = 0x2A; // 42 bytes
inline constexpr std::size_t kProjectileSaveOffset = 0xC403;
inline constexpr std::size_t kProjectileSaveBytes = 0x150; // 8 * 42
inline constexpr std::size_t kProjectileEmbeddedObjectOffset = 0x0E;
inline constexpr std::size_t kProjectileMovementBytes = 0x0E;
inline constexpr int kProjectileGuardHitTolerance = 9;
inline constexpr int kProjectileRenderDistanceThreshold = 20;

enum class ProjectileSlotState : std::uint8_t {
    Free = 0,
    Flying = 1,
    Impact = 2,
};

#pragma pack(push, 1)
struct ProjectileRuntimeRecord {
    std::int16_t xIsMajorAxis;       // +00: 1 when |X component| > |Y|, else 0
    std::int16_t lineError;          // +02: Bresenham error term
    std::int16_t minorErrorStep;     // +04: 2 * minor component
    std::int16_t majorErrorFixup;    // +06: 2 * (minor - major)
    std::int16_t stepX;              // +08: -1 or +1
    std::int16_t stepY;              // +0A: -1 or +1
    std::uint8_t state;              // +0C: Free / Flying / Impact
    std::uint8_t unknown0D;          // +0D: semantic TODO
    ObjectRuntimeRecord renderObject;// +0E: embedded 28-byte runtime OBJECT
};
#pragma pack(pop)

using ProjectilePool = std::array<ProjectileRuntimeRecord, kProjectilePoolCapacity>;

static_assert(sizeof(ProjectileRuntimeRecord) == kProjectileRecordStride);
static_assert(offsetof(ProjectileRuntimeRecord, xIsMajorAxis) == 0x00);
static_assert(offsetof(ProjectileRuntimeRecord, lineError) == 0x02);
static_assert(offsetof(ProjectileRuntimeRecord, minorErrorStep) == 0x04);
static_assert(offsetof(ProjectileRuntimeRecord, majorErrorFixup) == 0x06);
static_assert(offsetof(ProjectileRuntimeRecord, stepX) == 0x08);
static_assert(offsetof(ProjectileRuntimeRecord, stepY) == 0x0A);
static_assert(offsetof(ProjectileRuntimeRecord, state) == 0x0C);
static_assert(offsetof(ProjectileRuntimeRecord, renderObject) == kProjectileEmbeddedObjectOffset);
static_assert(kProjectilePoolCapacity * kProjectileRecordStride == kProjectileSaveBytes);
static_assert(kProjectileSaveOffset + kProjectileSaveBytes == 0xC553);
static_assert(kProjectileEmbeddedObjectOffset +
              offsetof(ObjectRuntimeRecord, runtime1A) == 0x28);

struct ProjectileSequenceOffsets {
    std::uint8_t flight;
    std::uint8_t impact;
};

// Weapon 2 is hitscan; selectors 0, 1 and 3 allocate projectile slots.
constexpr bool weaponUsesProjectile(std::uint8_t weaponSelector) noexcept {
    return weaponSelector == 0 || weaponSelector == 1 || weaponSelector == 3;
}

// Offsets within the projectile sequence bank, not global SEQDEF indices.
constexpr std::optional<ProjectileSequenceOffsets>
projectileSequenceOffsets(std::uint8_t weaponSelector) noexcept {
    switch (weaponSelector) {
    case 0: return ProjectileSequenceOffsets{0, 1};
    case 1: return ProjectileSequenceOffsets{2, 3};
    case 3: return ProjectileSequenceOffsets{0, 1};
    default: return std::nullopt;
    }
}

constexpr std::optional<std::size_t>
firstFreeProjectileSlot(const ProjectilePool& pool) noexcept {
    for (std::size_t i = 0; i < pool.size(); ++i) {
        if (pool[i].state == static_cast<std::uint8_t>(ProjectileSlotState::Free)) {
            return i;
        }
    }
    return std::nullopt;
}

// The guard collision test is performed in map/world coordinates after each
// small movement substep; both coordinate differences must be <= 9.
constexpr bool projectileHitsGuard(std::int32_t projectileX,
                                    std::int32_t projectileY,
                                    std::int32_t guardX,
                                    std::int32_t guardY) noexcept {
    // Widen before subtracting: the public int32_t inputs may span both limits.
    const auto dx = static_cast<std::int64_t>(projectileX) - guardX;
    const auto dy = static_cast<std::int64_t>(projectileY) - guardY;
    return dx >= -kProjectileGuardHitTolerance && dx <= kProjectileGuardHitTolerance &&
           dy >= -kProjectileGuardHitTolerance && dy <= kProjectileGuardHitTolerance;
}



struct ProjectileCollisionDecision {
    bool hitGuard;
    bool enterImpactState;
};

// The verified proximity test is the guard-hit decision point for the modern
// reconstruction. Class-specific damage/death remains in Combat/GuardSystem.
constexpr ProjectileCollisionDecision projectileGuardCollisionDecision(
    std::int32_t projectileX,
    std::int32_t projectileY,
    std::int32_t guardX,
    std::int32_t guardY) noexcept {
    const bool hit = projectileHitsGuard(projectileX, projectileY, guardX, guardY);
    return {hit, hit};
}

constexpr std::uint8_t projectileImpactStateValue() noexcept {
    return static_cast<std::uint8_t>(ProjectileSlotState::Impact);
}

// FUN_1010_9E20 uses this threshold only to decide whether to project the
// embedded OBJECT. It is not a lifetime, despawn, or slot-release rule.
constexpr bool projectileNeedsProjection(std::int32_t projectileX,
                                          std::int32_t projectileY,
                                          std::int32_t playerX,
                                          std::int32_t playerY) noexcept {
    // Preserve the axis-aligned threshold without signed subtraction overflow.
    const auto dx = static_cast<std::int64_t>(projectileX) - playerX;
    const auto dy = static_cast<std::int64_t>(projectileY) - playerY;
    return dx > kProjectileRenderDistanceThreshold || dx < -kProjectileRenderDistanceThreshold ||
           dy > kProjectileRenderDistanceThreshold || dy < -kProjectileRenderDistanceThreshold;
}

} // namespace nitemare3d::game
