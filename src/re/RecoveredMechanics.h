#pragma once

#include "re/OriginalRuntime.h"

#include <algorithm>
#include <cstdint>

namespace n3d::re {

struct RecoveredMechanics final {
    static constexpr std::uint8_t WallHardBlock = 0x04;
    static constexpr std::uint8_t WallDynamicDoor = 0x08;
    static constexpr std::uint8_t WallScriptTouch = 0x40;

    static constexpr std::uint8_t ObjectRuntimePresent = 0x01;
    static constexpr std::uint8_t ObjectBlocksMovement = 0x02;
    static constexpr std::uint8_t ObjectSpecialTouch = 0x04;
    static constexpr std::uint8_t ObjectCreatesGuard = 0x08;

    static constexpr int ObjectFlagsOffset = 0x05;
    static constexpr int ObjectClassOffset = 0x06;
    static constexpr int ObjectGuardIndexOffset = 0x07;
    static constexpr int ObjectMapBindingOffset = 0x0C;
    static constexpr int ObjectWorldXOffset = 0x10;
    static constexpr int ObjectWorldYOffset = 0x12;
    static constexpr int ObjectProjectedDamageBaselineOffset = 0x18;

    static constexpr int GuardTimerOffset = 0x06;
    static constexpr int GuardObjectSlotOffset = 0x08;
    static constexpr int GuardStrategyOffset = 0x0A;
    static constexpr int GuardStateOffset = 0x0B;
    static constexpr int GuardNextStateOffset = 0x0C;
    static constexpr int GuardStrengthOffset = 0x10;

    static constexpr int clampHealth(int health) noexcept {
        return std::clamp(health, 0, OriginalRuntime::PlayerMaxHealth);
    }

    static constexpr int clampNormalAmmo(int ammo) noexcept {
        return std::clamp(ammo, 0, OriginalRuntime::NormalAmmoCap);
    }

    static constexpr bool hasInput(std::uint16_t mask, std::uint16_t bit) noexcept {
        return (mask & bit) != 0;
    }
};

} // namespace n3d::re
