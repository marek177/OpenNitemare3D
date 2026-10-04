#include "formats/Pcx8.hpp"
#include "formats/BinaryIO.hpp"

#include <algorithm>
#include <span>
#include <stdexcept>

namespace n3d {

Pcx8Image Pcx8::load(const std::filesystem::path& path) {
    return decode(io::readFile(path));
}

std::array<Rgb8, 256> Pcx8::extractPalette(const std::filesystem::path& path) {
    return decode(io::readFile(path)).palette;
}

Pcx8Image Pcx8::decode(const std::vector<std::uint8_t>& bytesVec) {
    const std::span<const std::uint8_t> bytes(bytesVec);
    if (bytes.size() < 128 + 769) {
        throw std::runtime_error("PCX file too small");
    }
    if (bytes[0] != 0x0A || bytes[2] != 1 || bytes[3] != 8) {
        throw std::runtime_error("Only 8-bit RLE PCX is currently supported");
    }

    const auto xmin = io::readU16LE(bytes, 4);
    const auto ymin = io::readU16LE(bytes, 6);
    const auto xmax = io::readU16LE(bytes, 8);
    const auto ymax = io::readU16LE(bytes, 10);
    if (xmax < xmin || ymax < ymin) {
        throw std::runtime_error("Invalid PCX dimensions");
    }

    const std::uint16_t width = static_cast<std::uint16_t>(xmax - xmin + 1);
    const std::uint16_t height = static_cast<std::uint16_t>(ymax - ymin + 1);
    const std::uint8_t planes = bytes[65];
    const std::uint16_t bytesPerLine = io::readU16LE(bytes, 66);
    if (planes != 1 || bytesPerLine < width) {
        throw std::runtime_error("Unsupported PCX plane layout");
    }

    const std::size_t paletteMarker = bytes.size() - 769;
    if (bytes[paletteMarker] != 0x0C) {
        throw std::runtime_error("256-color PCX palette marker missing");
    }

    Pcx8Image image;
    image.width = width;
    image.height = height;
    image.pixels.resize(static_cast<std::size_t>(width) * height);

    std::size_t src = 128;
    std::vector<std::uint8_t> row(bytesPerLine);
    for (std::size_t y = 0; y < height; ++y) {
        std::size_t x = 0;
        while (x < bytesPerLine) {
            if (src >= paletteMarker) {
                throw std::runtime_error("PCX RLE stream ended early");
            }
            std::uint8_t token = bytes[src++];
            std::size_t run = 1;
            std::uint8_t value = token;
            if ((token & 0xC0u) == 0xC0u) {
                run = token & 0x3Fu;
                if (src >= paletteMarker) {
                    throw std::runtime_error("PCX RLE run missing value");
                }
                value = bytes[src++];
            }
            if (x + run > bytesPerLine) {
                throw std::runtime_error("PCX RLE run exceeds scanline");
            }
            std::fill_n(row.begin() + static_cast<std::ptrdiff_t>(x), run, value);
            x += run;
        }
        std::copy_n(row.begin(), width,
                    image.pixels.begin() + static_cast<std::ptrdiff_t>(y * width));
    }

    for (std::size_t i = 0; i < 256; ++i) {
        image.palette[i].r = bytes[paletteMarker + 1 + i * 3];
        image.palette[i].g = bytes[paletteMarker + 1 + i * 3 + 1];
        image.palette[i].b = bytes[paletteMarker + 1 + i * 3 + 2];
    }
    return image;
}

} // namespace n3d
