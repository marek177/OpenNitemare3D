#pragma once

#include "game/GuardSystem.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>

namespace nitemare3d::game {

// A slot-relative view. This copies bytes instead of reinterpreting unaligned
// file data as a packed struct, preserving unknown fields exactly.
using GuardBytes = std::array<std::uint8_t, kGuardRecordSize>;
using GuardSaveBytes = std::array<GuardBytes, kGuardCapacity>;

inline GuardSaveBytes readGuardSaveBlock(std::span<const std::uint8_t> slot) {
    if (slot.size() < kGuardSaveOffset + kGuardSaveSize) {
        throw std::invalid_argument("USER.SAV slot ends before GUARD block");
    }
    GuardSaveBytes guards{};
    for (std::size_t i = 0; i < kGuardCapacity; ++i) {
        const auto begin = slot.begin() + static_cast<std::ptrdiff_t>(kGuardSaveOffset + i * kGuardRecordSize);
        std::copy_n(begin, kGuardRecordSize, guards[i].begin());
    }
    return guards;
}

inline void writeGuardSaveBlock(std::span<std::uint8_t> slot, const GuardSaveBytes& guards) {
    if (slot.size() < kGuardSaveOffset + kGuardSaveSize) {
        throw std::invalid_argument("USER.SAV slot ends before GUARD block");
    }
    for (std::size_t i = 0; i < kGuardCapacity; ++i) {
        auto begin = slot.begin() + static_cast<std::ptrdiff_t>(kGuardSaveOffset + i * kGuardRecordSize);
        std::copy(guards[i].begin(), guards[i].end(), begin);
    }
}

inline std::uint16_t guardTimer(const GuardBytes& guard) noexcept {
    return static_cast<std::uint16_t>(guard[0x06] | (static_cast<std::uint16_t>(guard[0x07]) << 8));
}

inline void setGuardTimer(GuardBytes& guard, std::uint16_t timer) noexcept {
    guard[0x06] = static_cast<std::uint8_t>(timer);
    guard[0x07] = static_cast<std::uint8_t>(timer >> 8);
}

} // namespace nitemare3d::game
