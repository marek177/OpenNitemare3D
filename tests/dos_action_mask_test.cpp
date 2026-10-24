#include "input/DosActionMask.hpp"
#include <array>
#include <cassert>
#include <cstdint>

int main() {
    using namespace n3d::input;
    constexpr std::array<std::uint16_t, 8> combinations{
        0x0000, 0x0080, 0x0100, 0x0200,
        0x0180, 0x0280, 0x0300, 0x0380
    };
    for (const auto expected : combinations) {
        DosActions state{};
        state.set(DosAction::Fire, (expected & 0x0080u) != 0);
        state.set(DosAction::Strafe, (expected & 0x0100u) != 0);
        state.set(DosAction::Use, (expected & 0x0200u) != 0);
        assert(state.action_mask() == expected);
        assert(state.pressed(DosAction::Fire) == ((expected & 0x80u) != 0));
        assert(state.pressed(DosAction::Strafe) == ((expected & 0x100u) != 0));
        assert(state.pressed(DosAction::Use) == ((expected & 0x200u) != 0));
        state.set(DosAction::Fire, false);
        state.set(DosAction::Strafe, false);
        state.set(DosAction::Use, false);
        assert(state.action_mask() == 0);
    }
    DosActions preserve{0x4001};
    preserve.set(DosAction::Use, true);
    assert(preserve.raw == 0x4201);
    preserve.set(DosAction::Use, false);
    assert(preserve.raw == 0x4001);
}
