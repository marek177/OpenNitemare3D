#pragma once

#include <cstdint>

namespace nitemare3d::game::health {

// Executable semantic subsets of docs/PLAYER_HEALTH_RE.md (Win16 V1.10).
// These are modern value objects, NOT the original packed runtime/save layout.
// Death animation, sounds, item consumption and caller dispatch are not modeled.
inline constexpr std::uint16_t kHealthAddress = 0x4C1D;
inline constexpr std::uint16_t kGameStateAddress = 0x46B4;
inline constexpr std::uint8_t kMaximumHealth = 100;
inline constexpr std::uint16_t kEnemyDeathState = 2;
inline constexpr std::uint16_t kResetOffset = 0xBA30;          // original segment 3
inline constexpr std::uint16_t kHudClampOffset = 0xA4CE;      // original segment 3
inline constexpr std::uint16_t kEnemyReceiverOffset = 0x8C09; // original segment 3

struct State {
    std::uint8_t value{kMaximumHealth};
    std::uint16_t gameState{}; // logical value of DS:46B4, not a size assertion
    bool omnipotent{};
};

enum class DamageResult {
    SuppressedByOmnipotent,
    SuppressedByState2,
    NonLethal,
    Lethal,
};

// damage is the final byte produced by A1EA; do NOT scale for difficulty again.
// Deliberately no generic "health == 0" or "state >= 2" early return: the
// recovered gate is specifically state == 2, followed by unsigned damage >= HP.
[[nodiscard]] constexpr DamageResult applyEnemyDamage(State& state,
                                                       std::uint8_t damage) noexcept {
    if (state.omnipotent) return DamageResult::SuppressedByOmnipotent;
    if (state.gameState == kEnemyDeathState) return DamageResult::SuppressedByState2;
    if (damage >= state.value) {
        state.value = 0;
        state.gameState = kEnemyDeathState;
        return DamageResult::Lethal; // caller still owes the death-transition effects
    }
    state.value = static_cast<std::uint8_t>(state.value - damage);
    return DamageResult::NonLethal;
}

// A4CE performs an unsigned min-with-100 AND writes it back, not just a display
// clamp. Pickups can leave a transient 101..129 before this separate operation.
constexpr std::uint8_t clampForHud(State& state) noexcept {
    if (state.value > kMaximumHealth) state.value = kMaximumHealth;
    return state.value;
}

enum class FixedPickup : std::uint8_t { Add20 = 20, Add30 = 30 };

// Only the fixed health-write branches at 1CD2 and D048 are represented.
// The shifted D018 branch, visible item names and any score/item side effects
// remain outside this helper. True means a health write, not item consumption.
[[nodiscard]] constexpr bool applyFixedPickup(State& state, FixedPickup pickup) noexcept {
    if (pickup != FixedPickup::Add20 && pickup != FixedPickup::Add30) return false;
    if (state.value >= kMaximumHealth) return false;
    state.value = static_cast<std::uint8_t>(state.value + static_cast<std::uint8_t>(pickup));
    return true;
}

// Shared value write seen at reset/restore/cheat sites. Not a respawn: do not
// silently reset the independent DS:46B4 state or the Omnipotent flag.
constexpr void restoreTo100(State& state) noexcept { state.value = kMaximumHealth; }

} // namespace nitemare3d::game::health
