#pragma once

#include <cstddef>
#include <cstdint>

namespace n3d::img {

inline constexpr std::size_t kImageIndexDirectoryBytes = 0x400;
inline constexpr std::size_t kImageIndexEntries = 0x100;
inline constexpr std::size_t kWallImageDirectoryOffset = 0x000;
inline constexpr std::size_t kObjectImageDirectoryOffset = 0x400;
inline constexpr std::size_t kImageIndexRegionEnd = 0x800;

inline constexpr std::size_t kSequenceDefinitionBytes = 0x5A;
inline constexpr std::size_t kSequenceDefinitionsPerBank = 0x100;
inline constexpr std::size_t kLowSequenceBankOffset = 0x0008;
inline constexpr std::size_t kHighSequenceBankOffset =
    kLowSequenceBankOffset + kSequenceDefinitionsPerBank * kSequenceDefinitionBytes;
inline constexpr std::size_t kHighSequenceBankEnd =
    kHighSequenceBankOffset + kSequenceDefinitionsPerBank * kSequenceDefinitionBytes;

enum class SequenceBank : std::uint8_t {
    LowSelector,
    HighSelector,
};

constexpr std::size_t sequenceDefinitionOffset(SequenceBank bank,
                                              std::uint8_t selector) noexcept {
    const std::size_t bankIndex =
        bank == SequenceBank::HighSelector ? kSequenceDefinitionsPerBank : 0;
    return kLowSequenceBankOffset +
           (bankIndex + static_cast<std::size_t>(selector)) * kSequenceDefinitionBytes;
}

constexpr bool sequenceDefinitionOverlapsImageDirectories(SequenceBank bank,
                                                           std::uint8_t selector) noexcept {
    const auto begin = sequenceDefinitionOffset(bank, selector);
    const auto end = begin + kSequenceDefinitionBytes;
    return begin < kImageIndexRegionEnd && end > 0;
}

static_assert(kHighSequenceBankOffset == 0x5A08);
static_assert(kHighSequenceBankEnd == 0xB408);
static_assert(sequenceDefinitionOffset(SequenceBank::LowSelector, 1) == 0x0062);
static_assert(sequenceDefinitionOffset(SequenceBank::LowSelector, 23) == 0x081E);
static_assert(sequenceDefinitionOffset(SequenceBank::HighSelector, 0) == 0x5A08);
static_assert(sequenceDefinitionOffset(SequenceBank::HighSelector, 255) == 0xB3AE);
static_assert(sequenceDefinitionOverlapsImageDirectories(SequenceBank::LowSelector, 22));
static_assert(!sequenceDefinitionOverlapsImageDirectories(SequenceBank::LowSelector, 23));
static_assert(!sequenceDefinitionOverlapsImageDirectories(SequenceBank::HighSelector, 0));

} // namespace n3d::img
