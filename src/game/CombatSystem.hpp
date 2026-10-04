#pragma once

#include "game/DifficultySystem.hpp"

#include <algorithm>
#include <cstdint>

namespace nitemare3d::game {

// Clean-room combat constants recovered from NITE3W.EXE V1.10.
// Detailed evidence and class/weapon transform matrix live in
// docs/COMBAT_DAMAGE_RE.md.

enum class WeaponSelector : std::uint8_t {
    SingleShotLaser = 0,
    MagicWand = 1,
    SilverPistol = 2,
    ContinuousLaser = 3,
    None = 0xFF,
};

inline constexpr std::uint16_t kActiveWeaponGlobal = 0x4C23;
inline constexpr std::uint16_t kSilverAmmoGlobal = 0x4C1F;
inline constexpr std::uint16_t kLaserAmmoGlobal = 0x4C20;
inline constexpr std::uint16_t kWandAmmoGlobal = 0x4C44;
inline constexpr std::uint16_t kWeaponJamGlobal = 0x4C2E;
inline constexpr std::uint16_t kDifficultyGlobal = 0x4C14;

inline constexpr std::uint8_t kAmmoPickupAmount = 20;
inline constexpr std::uint8_t kAmmoForcedSetAmount = 50;
inline constexpr std::uint8_t kNormalAmmoCap = 100;
inline constexpr std::uint8_t kSignedAmmoPositiveMax = 127;

// Silver and laser pickup/HUD logic use signed-byte comparisons in the
// original executable. 100 is the intended normal cap; 127 is the highest
// positive signed value if memory is edited manually.
constexpr bool usesSignedAmmoSemantics(WeaponSelector weapon) noexcept {
    return weapon == WeaponSelector::SingleShotLaser ||
           weapon == WeaponSelector::SilverPistol ||
           weapon == WeaponSelector::ContinuousLaser;
}

constexpr std::uint16_t ammoGlobalForWeapon(WeaponSelector weapon) noexcept {
    switch (weapon) {
    case WeaponSelector::SingleShotLaser:
    case WeaponSelector::ContinuousLaser:
        return kLaserAmmoGlobal;
    case WeaponSelector::MagicWand:
        return kWandAmmoGlobal;
    case WeaponSelector::SilverPistol:
        return kSilverAmmoGlobal;
    default:
        return 0;
    }
}

// Damage producer at seg3:9FA2:
//   base = ((OBJECT+0x18) - viewReferenceY) * 8 + random()%25
// FUN_1010_CC7C writes a camera-projection/depth-scale cache to OBJECT+0x18;
// it is not world Y or a fixed weapon damage value. Class/weapon and difficulty
// transforms are applied to the raw result.
inline constexpr std::uint16_t kDamageProducerOffset = 0x9FA2;
inline constexpr std::uint8_t kDamageRandomRange = 25;
inline constexpr std::uint8_t kMaximumDamage = 255;

// Explicit enemy resistance table dispatch spans OBJECT class 0x0C..0x1F.
inline constexpr std::uint8_t kFirstDamageTableObjectClass = 0x0C;
inline constexpr std::uint8_t kLastDamageTableObjectClass = 0x1F;

// Special combat identities / transitions recovered from the executable.
inline constexpr std::uint8_t kDraculaPhase1Class = 0x11;
inline constexpr std::uint8_t kDraculaBatPhase2Class = 0x14;
inline constexpr std::uint8_t kPenelopeClass = 0x15;
inline constexpr std::uint8_t kHamersteinClass = 0x16;
inline constexpr std::uint8_t kCannonClass = 0x19;
inline constexpr std::uint8_t kGhostClass = 0x1A;

inline constexpr std::uint8_t kFreshGuardStrength = 0xFF;
inline constexpr std::uint8_t kPainState = 0x15;
inline constexpr std::uint8_t kPainResultOctant = 8;

// Dracula lethal phase-1 transition constants.
inline constexpr std::uint8_t kDraculaPhase2State = 0x08;
inline constexpr std::uint8_t kDraculaPhase2NextState = 0x02;
inline constexpr std::uint16_t kDraculaPhase2Timer = 1;
inline constexpr std::uint8_t kDraculaPhase2SequenceValue = 0x23;
inline constexpr std::uint8_t kDraculaTransformEvent = 0x22;

// Hamerstein normal damage producer returns literal 3 only while the special
// global gate equals 3; otherwise it returns zero. Narrative meaning of the
// gate remains PARTIAL.
inline constexpr std::uint16_t kHamersteinGateGlobal = 0x7E52;
inline constexpr std::uint8_t kHamersteinGateRequiredValue = 3;
inline constexpr std::uint8_t kHamersteinBaseDamage = 3;

// E1M9 scripted weapon jam events.
inline constexpr std::uint8_t kWeaponJamEnableEvent = 0x47;
inline constexpr std::uint8_t kWeaponJamDisableEvent = 0x48;
inline constexpr std::uint8_t kWeaponJamSoundId = 0x44;



enum class DamageTransformKind : std::uint8_t {
    PassThrough,
    Divide2,
    Divide4,
    Divide8,
    Divide16,
    Divide256,
    Immune,
    HamersteinGate,
};

// Direct transcription of the verified class x weapon transform at seg3:9FE8.
// A divisor of 256 is literal behavior in the original, not a symbolic immunity.
constexpr DamageTransformKind damageTransformFor(std::uint8_t objectClass,
                                                 WeaponSelector weapon) noexcept {
    const bool wand = weapon == WeaponSelector::MagicWand;
    const bool silver = weapon == WeaponSelector::SilverPistol;

    switch (objectClass) {
    case 0x0C: return DamageTransformKind::Divide8;
    case 0x0D: return wand ? DamageTransformKind::Divide2 : DamageTransformKind::Divide8;
    case 0x0E:
        return (wand || silver) ? DamageTransformKind::Divide2
                                : DamageTransformKind::Divide8;
    case 0x0F:
    case 0x10:
        return wand ? DamageTransformKind::Divide2 : DamageTransformKind::Divide256;
    case 0x11:
    case 0x14:
        return (wand || silver) ? DamageTransformKind::Divide2
                                : DamageTransformKind::Divide8;
    case 0x12:
    case 0x13:
        return DamageTransformKind::Divide4;
    case 0x15:
    case 0x19:
        return DamageTransformKind::Immune;
    case 0x16:
        return DamageTransformKind::HamersteinGate;
    case 0x17:
        return wand ? DamageTransformKind::Divide256 : DamageTransformKind::Divide4;
    case 0x18:
        if (wand) return DamageTransformKind::Divide256;
        if (silver) return DamageTransformKind::Divide16;
        return DamageTransformKind::Divide8;
    case 0x1A:
        return wand ? DamageTransformKind::Divide2 : DamageTransformKind::Immune;
    case 0x1B:
    case 0x1C:
        return DamageTransformKind::Divide2;
    case 0x1D:
        return DamageTransformKind::Divide8;
    case 0x1E:
        return wand ? DamageTransformKind::Immune : DamageTransformKind::Divide8;
    case 0x1F:
        return wand ? DamageTransformKind::Immune : DamageTransformKind::Divide4;
    default:
        return DamageTransformKind::PassThrough;
    }
}

constexpr int applyDamageTransform(int rawDamage,
                                   DamageTransformKind transform,
                                   std::uint8_t hamersteinGateValue = 0) noexcept {
    if (rawDamage <= 0)
        return rawDamage;

    switch (transform) {
    case DamageTransformKind::PassThrough: return rawDamage;
    case DamageTransformKind::Divide2: return rawDamage / 2;
    case DamageTransformKind::Divide4: return rawDamage / 4;
    case DamageTransformKind::Divide8: return rawDamage / 8;
    case DamageTransformKind::Divide16: return rawDamage / 16;
    case DamageTransformKind::Divide256: return rawDamage / 256;
    case DamageTransformKind::Immune: return 0;
    case DamageTransformKind::HamersteinGate:
        return hamersteinGateValue == kHamersteinGateRequiredValue
            ? kHamersteinBaseDamage
            : 0;
    }
    return rawDamage;
}

constexpr int playerDamageAfterResistanceAndDifficulty(
    int rawDamage,
    std::uint8_t objectClass,
    WeaponSelector weapon,
    Difficulty difficulty,
    std::uint8_t hamersteinGateValue = 0) noexcept {
    int damage = applyDamageTransform(
        rawDamage, damageTransformFor(objectClass, weapon), hamersteinGateValue);
    damage = scalePlayerDamageByDifficulty(damage, difficulty);
    return damage > kMaximumDamage ? kMaximumDamage : damage;
}

// The receiver compares positive damage against GUARD+0x10 strength.
// Lethal damage clears strength and enters death/special handling; non-lethal
// damage subtracts strength, writes resultOctant=8 and enters state 0x15.
constexpr std::uint8_t subtractGuardStrength(std::uint8_t strength,
                                             int damage) noexcept {
    if (damage <= 0) {
        return strength;
    }
    if (damage >= strength) {
        return 0;
    }
    return static_cast<std::uint8_t>(strength - damage);
}

static_assert(ammoGlobalForWeapon(WeaponSelector::SingleShotLaser) == kLaserAmmoGlobal);
static_assert(ammoGlobalForWeapon(WeaponSelector::ContinuousLaser) == kLaserAmmoGlobal);
static_assert(ammoGlobalForWeapon(WeaponSelector::MagicWand) == kWandAmmoGlobal);
static_assert(ammoGlobalForWeapon(WeaponSelector::SilverPistol) == kSilverAmmoGlobal);
static_assert(subtractGuardStrength(255, 0) == 255);
static_assert(subtractGuardStrength(255, 10) == 245);
static_assert(subtractGuardStrength(10, 10) == 0);

} // namespace nitemare3d::game
