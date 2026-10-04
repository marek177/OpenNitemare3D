#pragma once

#include <cstdint>

namespace nitemare3d::game {

// Clean-room constants recovered from NITE3W.EXE / MAP.* wall-class analysis.
// Only directly supported values are encoded here; unresolved frame timing and
// completion behavior stay documented as unknown rather than guessed.

enum class RecoveredWallClass : std::uint8_t {
    ExplodableWall1 = 0x2E,
    ExplodableWall2 = 0x2F,
};

inline constexpr std::uint8_t kWallPropertyExplodable = 0x10;

// Projectile-wall dispatch evidence:
// property 0x10 -> wall classes 0x2E/0x2F -> SFX event 0x29 -> runtime class 0x2D.
// The exact frame delay and final collision/map-cell cleanup routine are still
// open and must be confirmed dynamically before being promoted to constants.
inline constexpr std::uint8_t kExplodableWallSfxEvent = 0x29;
inline constexpr std::uint8_t kExplodableWallRuntimeClass = 0x2D;

constexpr bool isExplodableWallClass(std::uint8_t wallClass) noexcept {
    return wallClass == static_cast<std::uint8_t>(RecoveredWallClass::ExplodableWall1) ||
           wallClass == static_cast<std::uint8_t>(RecoveredWallClass::ExplodableWall2);
}

struct AnimatedWallUnknowns {
    // Deliberately left as documentation markers. Do not assign values until a
    // runtime watchpoint / exact EXE branch proves them.
    static constexpr bool frameDelayKnown = false;
    static constexpr bool completionCleanupKnown = false;
    static constexpr bool loopModeKnownForAllClasses = false;
};

} // namespace nitemare3d::game
