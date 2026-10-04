#include "game/ProjectileRuntime.hpp"
#include "game/HazardSystem.hpp"

#include <cassert>
#include <cstdint>

int main() {
    using namespace nitemare3d::game;
    using n3d::FireHazardClass;
    using n3d::fireHazardBehavior;
    using n3d::fireHazardClassForObjectId;

    static_assert(sizeof(ProjectileRuntimeRecord) == 42);
    static_assert(kProjectilePoolCapacity * kProjectileRecordStride == 336);
    static_assert(kProjectileSaveOffset == 0xC403);
    static_assert(kProjectileEmbeddedObjectOffset == 0x0E);
    static_assert(weaponUsesProjectile(0));
    static_assert(weaponUsesProjectile(1));
    static_assert(!weaponUsesProjectile(2));
    static_assert(weaponUsesProjectile(3));
    static_assert(projectileSequenceOffsets(1)->flight == 2);
    static_assert(projectileSequenceOffsets(1)->impact == 3);
    static_assert(!projectileSequenceOffsets(2).has_value());

    assert(projectileHitsGuard(9, -9, 0, 0));
    assert(!projectileHitsGuard(10, 0, 0, 0));
    assert(projectileNeedsProjection(21, 0, 0, 0));
    assert(projectileNeedsProjection(0, -21, 0, 0));
    assert(!projectileNeedsProjection(20, -20, 0, 0));

    ProjectilePool pool{};
    for (auto& projectile : pool)
        projectile.state = static_cast<std::uint8_t>(ProjectileSlotState::Free);
    assert(firstFreeProjectileSlot(pool) == 0);
    pool[0].state = static_cast<std::uint8_t>(ProjectileSlotState::Flying);
    assert(firstFreeProjectileSlot(pool) == 1);
    for (auto& projectile : pool)
        projectile.state = static_cast<std::uint8_t>(ProjectileSlotState::Impact);
    assert(!firstFreeProjectileSlot(pool));

    const auto small = fireHazardBehavior(FireHazardClass::Small);
    const auto medium = fireHazardBehavior(FireHazardClass::Medium);
    const auto large = fireHazardBehavior(FireHazardClass::Large);
    assert(small.passable && small.damagePerSimulationUpdate == 2);
    assert(medium.passable && medium.damagePerSimulationUpdate == 10);
    assert(!large.passable && large.damagePerSimulationUpdate == 100);
    assert(fireHazardClassForObjectId(0x3D) == FireHazardClass::Small);
    assert(fireHazardClassForObjectId(0x3C) == FireHazardClass::Medium);
    assert(fireHazardClassForObjectId(0x3B) == FireHazardClass::Large);
    assert(fireHazardClassForObjectId(0xFF) == FireHazardClass::None);
}
