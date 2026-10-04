#include "formats/MapArchive.hpp"
#include "formats/BinaryIO.hpp"

#include <algorithm>
#include <stdexcept>

namespace n3d {

MapArchive MapArchive::load(const std::filesystem::path& path) {
    const auto bytes = io::readFile(path);
    if (bytes.size() < HeaderSize) {
        throw std::runtime_error("MAP file smaller than 514-byte header: " + path.string());
    }

    const auto payloadSize = bytes.size() - HeaderSize;
    if (payloadSize % LevelBytes != 0) {
        throw std::runtime_error("MAP payload is not an integer number of 8192-byte levels: " + path.string());
    }

    MapArchive out;
    std::copy_n(bytes.begin(), HeaderSize, out.rawHeader_.begin());
    out.declaredLevelCount_ = io::readU16LE(bytes, 0);

    const std::size_t actualLevelCount = payloadSize / LevelBytes;
    if (out.declaredLevelCount_ != actualLevelCount) {
        throw std::runtime_error("MAP header level count does not match file length: " + path.string());
    }

    out.levels_.resize(actualLevelCount);
    for (std::size_t level = 0; level < actualLevelCount; ++level) {
        const std::size_t base = HeaderSize + level * LevelBytes;
        auto& dst = out.levels_[level];
        for (std::size_t i = 0; i < LevelMap::Width * LevelMap::Height; ++i) {
            dst.cells[i].wall = bytes[base + i * 2];
            dst.cells[i].object = bytes[base + i * 2 + 1];
        }
    }
    return out;
}

} // namespace n3d
