#pragma once

#include <cstdint>

namespace nitemare3d::game {

// Recovered from NITE3W.EXE V1.10.
// Global word 0x4C14 is the numeric difficulty selector.
// Exact displayed label strings still need final binding, so the enum names
// intentionally describe behavior rather than claiming original UI wording.
enum class Difficulty : std::uint16_t {
    Easier = 0,
    Baseline = 1,
    Harder = 2,
};

// Player -> enemy damage scaling, applied after class/weapon resistance.
constexpr int scalePlayerDamageByDifficulty(int damage, Difficulty difficulty) noexcept {
    switch (difficulty) {
        case Difficulty::Easier:
            return damage * 2;
        case Difficulty::Baseline:
            return damage;
        case Difficulty::Harder:
            return damage / 2;
    }
    return damage;
}

// Enemy -> player damage uses the inverse trend.
constexpr int scaleEnemyDamageByDifficulty(int damage, Difficulty difficulty) noexcept {
    switch (difficulty) {
        case Difficulty::Easier:
            return damage / 2;
        case Difficulty::Baseline:
            return damage;
        case Difficulty::Harder:
            return damage * 2;
    }
    return damage;
}

// GUARD timing uses the same global inversely: easier is slower, harder faster.
// The exact per-handler arithmetic varies, so expose only the recovered trend
// here instead of inventing a single universal timer formula.
enum class GuardTimingTrend : std::uint8_t {
    Slower,
    Baseline,
    Faster,
};

constexpr GuardTimingTrend guardTimingTrend(Difficulty difficulty) noexcept {
    switch (difficulty) {
        case Difficulty::Easier:
            return GuardTimingTrend::Slower;
        case Difficulty::Baseline:
            return GuardTimingTrend::Baseline;
        case Difficulty::Harder:
            return GuardTimingTrend::Faster;
    }
    return GuardTimingTrend::Baseline;
}

inline constexpr std::uint16_t kDifficultyGlobalOffset = 0x4C14;

static_assert(scalePlayerDamageByDifficulty(20, Difficulty::Easier) == 40);
static_assert(scalePlayerDamageByDifficulty(20, Difficulty::Baseline) == 20);
static_assert(scalePlayerDamageByDifficulty(20, Difficulty::Harder) == 10);
static_assert(scaleEnemyDamageByDifficulty(20, Difficulty::Easier) == 10);
static_assert(scaleEnemyDamageByDifficulty(20, Difficulty::Harder) == 40);

} // namespace nitemare3d::game
