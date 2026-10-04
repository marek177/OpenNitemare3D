#include "formats/BinaryIO.hpp"

#include <fstream>

namespace n3d::io {

std::vector<std::uint8_t> readFile(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) {
        throw std::runtime_error("Unable to open file: " + path.string());
    }

    const auto end = f.tellg();
    if (end < 0) {
        throw std::runtime_error("Unable to determine file size: " + path.string());
    }

    std::vector<std::uint8_t> data(static_cast<std::size_t>(end));
    f.seekg(0, std::ios::beg);
    if (!data.empty()) {
        f.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
    }
    if (!f && !data.empty()) {
        throw std::runtime_error("Failed while reading file: " + path.string());
    }
    return data;
}

void requireRange(std::span<const std::uint8_t> bytes,
                  std::size_t offset,
                  std::size_t length,
                  const std::string& what) {
    if (offset > bytes.size() || length > bytes.size() - offset) {
        throw std::runtime_error(what + ": range exceeds file size");
    }
}

std::uint16_t readU16LE(std::span<const std::uint8_t> bytes, std::size_t offset) {
    requireRange(bytes, offset, 2, "readU16LE");
    return static_cast<std::uint16_t>(bytes[offset]) |
           (static_cast<std::uint16_t>(bytes[offset + 1]) << 8u);
}

std::uint32_t readU32LE(std::span<const std::uint8_t> bytes, std::size_t offset) {
    requireRange(bytes, offset, 4, "readU32LE");
    return static_cast<std::uint32_t>(bytes[offset]) |
           (static_cast<std::uint32_t>(bytes[offset + 1]) << 8u) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 16u) |
           (static_cast<std::uint32_t>(bytes[offset + 3]) << 24u);
}

} // namespace n3d::io
