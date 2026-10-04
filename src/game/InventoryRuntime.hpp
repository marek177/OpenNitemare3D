#pragma once

#include <cstdint>

namespace nitemare3d::game {

// Runtime inventory globals recovered from NITE3W.EXE.
// Only addresses and bit relationships directly supported by executable/data
// analysis are promoted here. Human-readable color names are used where they
// are already bound by in-game text and WARP/key-gate behavior.
inline constexpr std::uintptr_t kColoredKeyMaskAddress = 0x4C28;
inline constexpr std::uintptr_t kIdCardMaskAddress    = 0x4C29;
inline constexpr std::uintptr_t kPentagramMaskAddress = 0x4C45;

// The colored-key and ID-card globals are bit masks. The ID-card ordering below
// is directly linked to the supplied Win16 OBJECTS/MAP data for the SECRET panel.
using InventoryMask = std::uint8_t;

enum class IdCardBit : std::uint8_t {
    Red = 0,
    Yellow = 1,
};

inline constexpr std::uint8_t kSecretPanelObjectClass = 0x03;
inline constexpr std::uint8_t kSecretPanelObjectId = 0x62;
inline constexpr std::uint8_t kSecretPanelRequiredIdCardObjectId = 0x09;
inline constexpr IdCardBit kSecretPanelRequiredIdCard = IdCardBit::Red;

constexpr bool hasInventoryBit(InventoryMask mask, std::uint8_t bit) noexcept {
    // The mask has exactly eight bits. Reject invalid indices before shifting;
    // shifting by the host unsigned-int width or more is undefined in C++.
    return bit < 8 && (mask & static_cast<InventoryMask>(1u << bit)) != 0;
}

constexpr void grantInventoryBit(InventoryMask& mask, std::uint8_t bit) noexcept {
    // Invalid indices are a no-op, not a modulo-width bit selection.
    if (bit < 8) {
        mask = static_cast<InventoryMask>(mask | static_cast<InventoryMask>(1u << bit));
    }
}

constexpr bool hasIdCard(InventoryMask mask, IdCardBit bit) noexcept {
    return hasInventoryBit(mask, static_cast<std::uint8_t>(bit));
}

constexpr bool hasSecretPanelCredential(InventoryMask idCardMask) noexcept {
    return hasIdCard(idCardMask, kSecretPanelRequiredIdCard);
}

// WARP_S1 uses all four pentagrams. This bit ordering is directly established
// by the reconstructed missing-item text builder.
enum class PentagramBit : std::uint8_t {
    Red    = 0,
    Green  = 1,
    Blue   = 2,
    Yellow = 3,
};

inline constexpr InventoryMask kAllPentagramsMask = 0x0F;

constexpr bool hasPentagram(InventoryMask mask, PentagramBit bit) noexcept {
    return hasInventoryBit(mask, static_cast<std::uint8_t>(bit));
}

constexpr bool hasAllPentagrams(InventoryMask mask) noexcept {
    return (mask & kAllPentagramsMask) == kAllPentagramsMask;
}

// WARP_L1..L4 share one generic key-gate routine. The wall class determines
// which key bit is required; keep the relation explicit rather than cloning
// four separate scripts.
inline constexpr std::uint8_t kWarpKeyClassBase = 0x19;
inline constexpr std::uint8_t kWarpKeyClassLast = 0x1C;

constexpr std::uint8_t warpKeyBitForClass(std::uint8_t wallClass) noexcept {
    return static_cast<std::uint8_t>(wallClass - kWarpKeyClassBase);
}

constexpr bool isWarpKeyClass(std::uint8_t wallClass) noexcept {
    return wallClass >= kWarpKeyClassBase && wallClass <= kWarpKeyClassLast;
}

} // namespace nitemare3d::game
