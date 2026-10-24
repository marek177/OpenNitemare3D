#pragma once
#include <cstdint>

namespace n3d::input {
// Observed DOS v2.0 held-key mask bits; see V085/V086 original guest-RAM traces.
// This is a pure input representation, NOT a verified gameplay tick scheduler.
enum class DosAction : std::uint16_t {
    Fire = 0x0080,
    Strafe = 0x0100, // Alt
    Use = 0x0200     // Space
};

constexpr std::uint16_t action_bit(DosAction a) noexcept {
    return static_cast<std::uint16_t>(a);
}

struct DosActions {
    std::uint16_t raw = 0;

    constexpr bool pressed(DosAction action) const noexcept {
        return (raw & action_bit(action)) != 0;
    }
    constexpr void set(DosAction action, bool down) noexcept {
        const auto bit = action_bit(action);
        raw = down ? static_cast<std::uint16_t>(raw | bit)
                   : static_cast<std::uint16_t>(raw & static_cast<std::uint16_t>(~bit));
    }
    constexpr std::uint16_t action_mask() const noexcept {
        return static_cast<std::uint16_t>(raw & 0x0380u);
    }
};
} // namespace n3d::input
