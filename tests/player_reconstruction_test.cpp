#include "game/InventoryRuntime.hpp"
#include "game/PlayerRuntime.hpp"
#include "game/PlayerHealthRuntime.hpp"
#include "game/PlayerCollisionRuntime.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>

namespace {
struct Checks {
    std::size_t count{}, failures{};
    void require(bool ok, const char* message) {
        ++count;
        if (!ok) {
            if (failures < 8) std::cerr << "FAIL: " << message << '\n';
            ++failures;
        }
    }
};

void checkHealth(Checks& c) {
    namespace h = nitemare3d::game::health;
    // Exhaust all byte HP/damage values, both immunity values, and states 0..3.
    // This includes raw HP >100 and the unusual zero-HP/zero-damage boundary.
    for (unsigned hp = 0; hp < 256; ++hp) {
        for (unsigned damage = 0; damage < 256; ++damage) {
            for (unsigned mode = 0; mode < 4; ++mode) {
                for (unsigned immunity = 0; immunity < 2; ++immunity) {
                    h::State state{static_cast<std::uint8_t>(hp),
                                   static_cast<std::uint16_t>(mode), immunity != 0};
                    const auto outcome = h::applyEnemyDamage(state, static_cast<std::uint8_t>(damage));
                    unsigned expectedHp = hp, expectedMode = mode;
                    auto expectedOutcome = h::DamageResult::NonLethal;
                    if (immunity) expectedOutcome = h::DamageResult::SuppressedByOmnipotent;
                    else if (mode == 2) expectedOutcome = h::DamageResult::SuppressedByState2;
                    else if (damage < hp) expectedHp = hp - damage;
                    else {
                        expectedHp = 0; expectedMode = 2;
                        expectedOutcome = h::DamageResult::Lethal;
                    }
                    c.require(state.value == expectedHp, "unsigned receiver HP result");
                    c.require(state.gameState == expectedMode, "receiver state gate/transition");
                    c.require(outcome == expectedOutcome, "receiver outcome");
                    c.require(state.omnipotent == (immunity != 0), "receiver preserves immunity flag");
                }
            }
        }
        for (unsigned amount : {20u, 30u}) {
            h::State state{static_cast<std::uint8_t>(hp), 3, true};
            const bool wrote = h::applyFixedPickup(state, static_cast<h::FixedPickup>(amount));
            const auto transient = hp < 100 ? hp + amount : hp;
            c.require(wrote == (hp < 100), "pickup threshold before addition");
            c.require(state.value == transient, "pickup does not prematurely clamp");
            c.require(h::clampForHud(state) == (transient > 100 ? 100 : transient), "unsigned HUD clamp");
            c.require(state.value == (transient > 100 ? 100 : transient), "HUD clamp writes back");
            c.require(state.gameState == 3 && state.omnipotent, "pickup/HUD preserve unrelated state");
            h::restoreTo100(state);
            c.require(state.value == 100 && state.gameState == 3 && state.omnipotent,
                      "restore is not a respawn");
        }
    }
    for (unsigned raw = 0; raw < 256; ++raw) {
        if (raw == 20 || raw == 30) continue;
        h::State state{50, 1, false};
        c.require(!h::applyFixedPickup(state, static_cast<h::FixedPickup>(raw)) && state.value == 50,
                  "unsupported fixed pickup rejected");
    }
    h::State overheal{99, 1, false};
    c.require(h::applyFixedPickup(overheal, h::FixedPickup::Add20) && overheal.value == 119,
              "99 + 20 remains 119 until HUD update");
    c.require(h::clampForHud(overheal) == 100, "119 then clamps to 100");
}

void checkCollision(Checks& c) {
    namespace m = nitemare3d::game::collision;
    // Independent expected tables, assembled from disjoint intervals rather
    // than copying the production bitwise builder.
    m::ByteTable walls{}, objects{}, types{};
    for (unsigned i = 1; i <= 0x30; ++i) walls[i] = 0x07;
    walls[0x2E] = walls[0x2F] = 0x17;
    for (unsigned i = 0x31; i <= 0x40; ++i) walls[i] = 0x0B;
    walls[0x47] = walls[0x48] = 0x40;
    objects[4] = 0x40;
    objects[6] = objects[7] = 0x01;
    for (unsigned i = 8; i <= 0x25; ++i) objects[i] = 0x0B;
    for (unsigned i = 0x26; i <= 0x2D; ++i) objects[i] = 0x03;
    objects[0x2A] = 0x23;
    objects[0x2E] = 0x01;
    for (unsigned i = 0x2F; i <= 0x3D; ++i) objects[i] = 0x05;
    for (unsigned i = 0; i < 256; ++i) {
        c.require(m::wallPropertiesForMappedType(static_cast<std::uint8_t>(i)) == walls[i], "wall flags");
        c.require(m::objectPropertiesForMappedType(static_cast<std::uint8_t>(i)) == objects[i], "object flags");
        types[i] = static_cast<std::uint8_t>(i * 73 + 19);
    }
    const auto wt = m::buildWallProperties(types);
    const auto ot = m::buildObjectProperties(types);
    for (unsigned i = 0; i < 256; ++i) {
        c.require(wt[i] == walls[types[i]], "wall ID is mapped before flags");
        c.require(ot[i] == objects[types[i]], "object ID is mapped before flags");
    }
    for (unsigned state = 0; state < 65536; ++state)
        c.require(m::doorStateAllowsPassage(static_cast<std::uint16_t>(state)) == (state == 0 || state == 4),
                  "only numeric door states 0/4 pass");
    for (std::int32_t world = -32768; world <= 32767; ++world)
        c.require(m::worldToTile(world) == static_cast<std::int32_t>(std::floor(world / 64.0)),
                  "signed world coordinate floor division");
    c.require(m::worldToTile(std::numeric_limits<std::int32_t>::min()) == -33554432, "INT32_MIN tile");
    c.require(m::worldToTile(std::numeric_limits<std::int32_t>::max()) == 33554431, "INT32_MAX tile");
    for (int y = -1; y <= 64; ++y) {
        for (int x = -1; x <= 64; ++x) {
            const auto offset = m::mapCellByteOffset(x, y);
            const bool valid = x >= 0 && y >= 0 && x < 64 && y < 64;
            c.require(offset.has_value() == valid, "MAP bounds contract");
            if (valid) c.require(*offset == y * 128 + x * 2, "two-byte MAP cell offset");
        }
    }
    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 64; ++x) {
            for (int within : {0, 27, 32, 63}) {
                const auto same = m::postMoveCell(x * 64 + within, y * 64 + within, x, y);
                c.require(same && same->tileX == x && same->tileY == y && !same->event,
                          "in-tile move emits no event");
                const auto entered = m::postMoveCell(x * 64 + within, y * 64 + within, x - 1, y);
                c.require(entered && entered->event == 0x16 && entered->byteOffset == (y * 64 + x) * 2,
                          "entering cell emits event 16 and binds MAP offset");
            }
        }
    }
    c.require(!m::postMoveCell(-1, 0, 0, 0), "negative world position rejected");
    c.require(!m::postMoveCell(4096, 0, 63, 0), "world beyond map rejected");
    c.require(!m::postMoveCell(0, 4096, 0, 63), "Y beyond map rejected");
}
} // namespace

int main() {
    using namespace nitemare3d::game;
    static_assert(kPlayerHealthAddress == 0x4C1D && kPlayerGameStateAddress == 0x46B4);
    static_assert(kPlayerTileXAddress == 0x4BF2 && kPlayerTileYAddress == 0x4BF4);
    static_assert(kPlayerMapCellOffsetAddress == 0x4C10 && kPlayerMapCellSegmentAddress == 0x4C12);
    static_assert(collision::kPlayerHalfExtent == 27 && collision::kWorldUnitsPerTile == 64);
    static_assert(collision::worldToTile(-1) == -1 && collision::worldToTile(64) == 1);
    static_assert(hasAllPentagrams(0x0F) && !hasAllPentagrams(0x07));
    static_assert(!hasInventoryBit(0xFF, 255));
    Checks checks;
    checkHealth(checks);
    checkCollision(checks);
    std::cout << "player_reconstruction_test: " << checks.count << " checks, "
              << checks.failures << " failures\n";
    return checks.failures == 0 ? 0 : 1;
}
