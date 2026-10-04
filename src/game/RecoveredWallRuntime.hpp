#pragma once

#include <cstddef>
#include <cstdint>

namespace nitemare3d::re {

// Recovered from Win16 NITE3W wall-class dispatch and runtime property generation.
// Only values supported by executable/data evidence are named here.
enum class WallRuntimeProperty : std::uint8_t {
    WorldGeometry = 0x01,
    CollisionRelevant = 0x02,
    SolidStaticInteractive = 0x04,
    Door = 0x08,
    Explodable = 0x10,
    Trigger = 0x40,
};

constexpr std::uint8_t operator|(WallRuntimeProperty a, WallRuntimeProperty b) noexcept {
    return static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b);
}

// Confirmed wall classes used by the central USE/door/trigger paths.
enum class RecoveredWallClass : std::uint8_t {
    LevelUp = 0x09,
    LevelUp2 = 0x0A,

    // WARP/portal dispatcher spans 0x0D..0x2C.
    WarpFirst = 0x0D,
    WarpLast = 0x2C,

    Explodable1 = 0x2E,
    Explodable2 = 0x2F,

    DoorFirst = 0x31,

    // Remote-controlled door family is known to live inside the door range;
    // exact retail display names remain documented separately.

    DoorCurtainVertical = 0x3F,   // DOORVC
    DoorCurtainHorizontal = 0x40, // DOORHC
    DoorLast = 0x40,

    Trigger1 = 0x47,
    Trigger2 = 0x48,
};

constexpr bool isDoorClass(std::uint8_t cls) noexcept {
    return cls >= static_cast<std::uint8_t>(RecoveredWallClass::DoorFirst) &&
           cls <= static_cast<std::uint8_t>(RecoveredWallClass::DoorLast);
}

constexpr bool isExplodableWallClass(std::uint8_t cls) noexcept {
    return cls == static_cast<std::uint8_t>(RecoveredWallClass::Explodable1) ||
           cls == static_cast<std::uint8_t>(RecoveredWallClass::Explodable2);
}

constexpr bool isTriggerClass(std::uint8_t cls) noexcept {
    return cls == static_cast<std::uint8_t>(RecoveredWallClass::Trigger1) ||
           cls == static_cast<std::uint8_t>(RecoveredWallClass::Trigger2);
}

constexpr bool isWarpClass(std::uint8_t cls) noexcept {
    return cls >= static_cast<std::uint8_t>(RecoveredWallClass::WarpFirst) &&
           cls <= static_cast<std::uint8_t>(RecoveredWallClass::WarpLast);
}

// Fixed-size runtime arrays recovered from the Win16 build.
inline constexpr std::size_t kMaxDoors = 64;
inline constexpr std::size_t kDoorRecordSize = 22;
inline constexpr std::size_t kMaxPanels = 32;
inline constexpr std::size_t kPanelRecordSize = 22;
inline constexpr std::size_t kMaxPushes = 12;
inline constexpr std::size_t kPushRecordSize = 6;
inline constexpr std::size_t kMaxGuards = 100;
inline constexpr std::size_t kGuardRecordSize = 26;
inline constexpr std::size_t kObservedObjectRecordSizeWin16 = 28;

// Addresses from the analysed Win16 image. They are documentation anchors,
// not portable pointers and must never be dereferenced by reconstructed code.
inline constexpr std::uint16_t kDoorArrayOffset = 0x9DD6;
inline constexpr std::uint16_t kPanelArrayOffset = 0xA356;

// Known central reverse-engineered routines (symbol names from analysis output):
// FUN_1010_19d6 : central USE dispatcher
// FUN_1010_22e8 : wall-class -> wall-ID lookup
// FUN_1010_234c : object-class lookup
// FUN_1010_2390 : runtime object lookup by class
// FUN_1010_23da : highest wall ID of class used in current 64x64 map
// FUN_1010_2470 : derive wall runtime property bytes
// FUN_1010_25ac : level-exit handling (LEVEL_UP / LEVEL_UP2)
// FUN_1010_272e : WARP family dispatcher
// FUN_1010_27b4 : shared teleport helper

} // namespace nitemare3d::re
