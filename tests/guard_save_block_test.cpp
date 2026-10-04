#include "game/GuardSaveBlock.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <vector>

int main() {
    using namespace nitemare3d::game;
    std::vector<std::uint8_t> slot(0xD6E7);
    for (std::size_t i = 0; i < slot.size(); ++i) slot[i] = static_cast<std::uint8_t>(i * 37);
    const auto original = slot;
    auto guards = readGuardSaveBlock(slot);
    assert(guardTimer(guards[99]) == static_cast<std::uint16_t>(
        original[kGuardSaveOffset + 99 * kGuardRecordSize + 6] |
        (original[kGuardSaveOffset + 99 * kGuardRecordSize + 7] << 8)));
    writeGuardSaveBlock(slot, guards);
    assert(slot == original);

    setGuardTimer(guards[99], 0x1234);
    writeGuardSaveBlock(slot, guards);
    const std::size_t pos = kGuardSaveOffset + 99 * kGuardRecordSize + 6;
    for (std::size_t i = 0; i < slot.size(); ++i) {
        const auto expected = i == pos ? 0x34 : i == pos + 1 ? 0x12 : original[i];
        assert(slot[i] == expected);
    }
    try {
        readGuardSaveBlock(std::span<const std::uint8_t>(slot.data(), kGuardSaveOffset + kGuardSaveSize - 1));
        assert(false);
    } catch (const std::invalid_argument&) {}
}
