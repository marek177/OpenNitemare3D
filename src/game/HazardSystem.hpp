#pragma once

#include <cstdint>

namespace n3d {

// Win16 NITE3W 1.10 maps object IDs 0x3B..0x3D to class-7 CAUSTIC fires.
// FUN_1010_BE62 applies these amounts per simulation update; DAT_53F2 is the
// calibrated update interval, so these are not fixed damage-per-second values.
// Passability comes from the E3M6 gameplay/video audit, independently of damage.
enum class FireHazardClass : std::uint8_t {
    None = 0,
    Small,
    Medium,
    Large,
};

inline constexpr std::uint8_t kLargeFireObjectId = 0x3B;
inline constexpr std::uint8_t kMediumFireObjectId = 0x3C;
inline constexpr std::uint8_t kSmallFireObjectId = 0x3D;

struct FireHazardBehavior {
    bool passable{};
    std::uint8_t damagePerSimulationUpdate{};
};

constexpr FireHazardClass fireHazardClassForObjectId(std::uint8_t objectId) noexcept {
    switch (objectId) {
    case kLargeFireObjectId: return FireHazardClass::Large;
    case kMediumFireObjectId: return FireHazardClass::Medium;
    case kSmallFireObjectId: return FireHazardClass::Small;
    default: return FireHazardClass::None;
    }
}

constexpr FireHazardBehavior fireHazardBehavior(FireHazardClass fire) noexcept {
    switch (fire) {
    case FireHazardClass::Small:
        return {true, 2};
    case FireHazardClass::Medium:
        return {true, 10};
    case FireHazardClass::Large:
        // 100 HP/update is lethal from the normal 100-HP maximum; the executable
        // damage path still expresses this as damage, not a separate kill flag.
        return {false, 100};
    case FireHazardClass::None:
    default:
        return {true, 0};
    }
}

} // namespace n3d
