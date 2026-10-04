#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace n3d {

struct Rgb8 {
    std::uint8_t r{}, g{}, b{};
};

struct Pcx8Image {
    std::uint16_t width{};
    std::uint16_t height{};
    std::vector<std::uint8_t> pixels;
    std::array<Rgb8, 256> palette{};
};

class Pcx8 {
public:
    static Pcx8Image decode(const std::vector<std::uint8_t>& bytes);
    static Pcx8Image load(const std::filesystem::path& path);
    static std::array<Rgb8, 256> extractPalette(const std::filesystem::path& path);
};

} // namespace n3d
