#include "formats/DemoFile.hpp"
#include "formats/BinaryIO.hpp"

#include <stdexcept>

namespace n3d {

DemoFile DemoFile::load(const std::filesystem::path& path) {
    const auto bytes = io::readFile(path);
    if (bytes.size() < 6 || (bytes.size() - 6) % 8 != 0) {
        throw std::runtime_error("Unexpected DEMO file size/layout: " + path.string());
    }

    DemoFile out;
    for (std::size_t i = 0; i < 3; ++i) {
        out.rawHeaderWords_[i] = io::readU16LE(bytes, i * 2);
    }

    std::uint32_t previousGeneration = 0;
    bool first = true;
    for (std::size_t pos = 6; pos < bytes.size(); pos += 8) {
        DemoRecord rec;
        rec.eventByte = bytes[pos];
        rec.inputMask = io::readU16LE(bytes, pos + 1);
        rec.paddingByte = bytes[pos + 3];
        rec.renderGeneration = io::readU32LE(bytes, pos + 4);
        if (!first && rec.renderGeneration < previousGeneration) {
            out.renderGenerationsAreMonotonic_ = false;
        }
        previousGeneration = rec.renderGeneration;
        first = false;
        out.records_.push_back(rec);
    }
    return out;
}

const DemoRecord* DemoFile::dispatchOneDueRecordAtGeneration(
    std::uint32_t currentRenderGeneration,
    std::size_t& nextRecordIndex) const noexcept {
    if (nextRecordIndex >= records_.size()) return nullptr;

    const auto& next = records_[nextRecordIndex];
    if (next.renderGeneration > currentRenderGeneration) return nullptr;

    ++nextRecordIndex;
    return &next;
}

} // namespace n3d
