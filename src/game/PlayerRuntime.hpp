#pragma once

#include "game/InventoryRuntime.hpp"
#include "game/PlayerHealthRuntime.hpp"

#include <cstdint>

namespace nitemare3d::game {

// Clean-room player/runtime globals recovered from NITE3W.EXE.
// Only addresses and meanings that are directly supported by the current
// reverse-engineering evidence are named here.

// Player world position commit sites observed in the original executable.
inline constexpr std::uint16_t kPlayerWorldXAddress = 0x4BF6; // VERIFIED_EXE
inline constexpr std::uint16_t kPlayerWorldYAddress = 0x4BF8; // VERIFIED_EXE

// seg3:8A20 commits the containing tile and MAP-cell far pointer.
inline constexpr std::uint16_t kPlayerTileXAddress = 0x4BF2;
inline constexpr std::uint16_t kPlayerTileYAddress = 0x4BF4;
inline constexpr std::uint16_t kPlayerMapCellOffsetAddress = 0x4C10;
inline constexpr std::uint16_t kPlayerMapCellSegmentAddress = 0x4C12;

// PLAYER_HEALTH_RE.md: initialization, clamp/writeback and damage receiver.
inline constexpr std::uint16_t kPlayerHealthAddress = health::kHealthAddress;
inline constexpr std::uint16_t kPlayerGameStateAddress = health::kGameStateAddress;

// Inventory addresses and pentagram helpers have a single definition in
// InventoryRuntime.hpp. Include it above so existing PlayerRuntime users keep
// access to those names without duplicate definitions in combined headers.

// Weapon / ammo runtime globals.
inline constexpr std::uint16_t kSilverAmmoAddress = 0x4C1F; // VERIFIED_EXE
inline constexpr std::uint16_t kLaserAmmoAddress  = 0x4C20; // VERIFIED_EXE, shared by weapon selectors 0 and 3
inline constexpr std::uint16_t kActiveWeaponAddress = 0x4C23; // VERIFIED_EXE
inline constexpr std::uint16_t kWeaponJamAddress = 0x4C2E; // VERIFIED_EXE, scripted jam flag
inline constexpr std::uint16_t kWandAmmoAddress = 0x4C44; // VERIFIED_EXE

// Difficulty and cheat/runtime flags.
inline constexpr std::uint16_t kDifficultyAddress = 0x4C14; // VERIFIED_EXE
inline constexpr std::uint16_t kOmnipotentAddress = 0x4BE5; // VERIFIED_EXE: bypasses ammo consumption path

// Normal pickup/display cap recovered from the ammo refill helper.
inline constexpr std::uint8_t kNormalAmmoCap = 100;
inline constexpr std::uint8_t kSignedAmmoSafeMax = 127; // silver/laser signed compare/display paths

// Active weapon selector values recovered from the damage/fire routines.
enum class PlayerWeapon : std::uint8_t {
    SingleShotLaser = 0,
    MagicWand = 1,
    SilverPistol = 2,
    ContinuousLaser = 3,
    None = 0xFF,
};

// Colored-key and ID-card masks are represented as runtime bitfields. Exact
// color-to-bit symbolic names are intentionally not encoded here until every
// door/WARP class is bound against retail data for all episodes.
struct AccessInventory {
    std::uint8_t coloredKeys = 0;
    std::uint8_t idCards = 0;
};

constexpr bool hasAccessBit(std::uint8_t mask, std::uint8_t bit) noexcept {
    return (mask & bit) != 0;
}

// Current confidence notes:
// - Player X/Y addresses: VERIFIED_EXE.
// - Key/card masks: VERIFIED_EXE through door/USE dispatch.
// - Ammo/weapon globals: VERIFIED_EXE through fire/ammo and damage routines.
// - Pentagram mask: VERIFIED_EXE through WARP_S1/S2 portal logic.
// - Health and tile/MAP-cell addresses: bound by PLAYER_HEALTH_RE.md and
//   PLAYER_COLLISION_RE.md; runnable semantic subsets are in PlayerHealthRuntime
//   and PlayerCollisionRuntime. No original-process pointers are dereferenced.
// - Score width and exact angle representation are not established here.

} // namespace nitemare3d::game
