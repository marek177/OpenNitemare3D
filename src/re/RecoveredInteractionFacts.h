#pragma once

#include <cstdint>

namespace n3d::re {

// Win16 reference facts. No runtime behavior is implemented here.
struct RecoveredInteractionFacts final {
    static constexpr std::uint8_t ControlWallClass = 0x03;
    static constexpr std::uint8_t ControlObjectClass = 0x03;
    static constexpr std::uint8_t RemoteDoorVerticalClass = 0x3B;
    static constexpr std::uint8_t RemoteDoorHorizontalClass = 0x3C;
    static constexpr std::uint8_t RemoteOpenCommand = 0x1E;
    static constexpr std::uint8_t RemoteCloseCommand = 0x1F;
    static constexpr std::uint8_t MovementScriptWallFlag = 0x40;
    static constexpr std::uint8_t PortalPentagramMask = 0x0F;
    static constexpr std::uint8_t PortalEntryClass = 0x15;
    static constexpr std::uint8_t PortalExitClass = 0x16;
    static constexpr std::uint8_t ExplodingWallRuntimeClass = 0x2D;
    static constexpr std::uint8_t ExplodingWallSound = 0x29;

    static constexpr bool isRemoteDoorClass(std::uint8_t value) noexcept {
        return value == RemoteDoorVerticalClass || value == RemoteDoorHorizontalClass;
    }

    static constexpr bool canOpenRemoteDoor(std::uint8_t state) noexcept {
        return state == 1 || state == 3;
    }

    static constexpr bool canCloseRemoteDoor(std::uint8_t state) noexcept {
        return state == 0 || state == 2;
    }

    static constexpr bool hasCard(std::uint16_t cards, std::uint8_t group) noexcept {
        return group < 16 && ((cards >> group) & 1u) != 0;
    }

    static constexpr bool hasAllPentagrams(std::uint8_t mask) noexcept {
        return (mask & PortalPentagramMask) == PortalPentagramMask;
    }
};

} // namespace n3d::re
