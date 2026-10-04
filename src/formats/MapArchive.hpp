#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace n3d {

struct MapCell {
    std::uint8_t wall{};
    std::uint8_t object{};
};

struct LevelMap {
    static constexpr std::size_t Width = 64;
    static constexpr std::size_t Height = 64;
    std::array<MapCell, Width * Height> cells{};

    const MapCell& at(std::size_t x, std::size_t y) const {
        return cells.at(y * Width + x);
    }
};

class MapArchive {
public:
    static constexpr std::size_t HeaderSize = 514;
    static constexpr std::size_t LevelBytes = 8192;

    static MapArchive load(const std::filesystem::path& path);

    std::uint16_t declaredLevelCount() const { return declaredLevelCount_; }
    const std::array<std::uint8_t, HeaderSize>& rawHeader() const { return rawHeader_; }
    const std::vector<LevelMap>& levels() const { return levels_; }

private:
    std::uint16_t declaredLevelCount_{};
    std::array<std::uint8_t, HeaderSize> rawHeader_{};
    std::vector<LevelMap> levels_;
};

} // namespace n3d
