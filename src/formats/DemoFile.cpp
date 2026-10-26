#include "formats/DemoFile.hpp"
#include "formats/BinaryIO.hpp"
#include <stdexcept>
namespace n3d {
DemoFile DemoFile::load(const std::filesystem::path& path) {
    const auto bytes = io::readFile(path);
    if (bytes.size() < 6 || (bytes.size() - 6) % 8 != 0)
        throw std::runtime_error("Unexpected DEMO file size/layout: " + path.string());
    DemoFile out;
    for (std::size_t i = 0; i < 3; ++i)
        out.rawHeaderWords_[i] = io::readU16LE(bytes, i * 2);
    std::uint32_t previousGeneration = 0;
    bool first = true;
    for (std::size_t pos = 6; pos < bytes.size(); pos += 8) {
        DemoRecord rec;
        rec.key = io::readU16LE(bytes, pos);
        rec.flags = io::readU16LE(bytes, pos + 2);
        rec.renderGeneration = io::readU32LE(bytes, pos + 4);
        if (!first && rec.renderGeneration < previousGeneration)
            out.renderGenerationsAreMonotonic_ = false;
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
bool DemoReaderState::applyDue(const DemoFile& file, std::uint32_t counter,
                               std::uint16_t& readReturnBytes) noexcept {
    const DemoRecord* applied = file.dispatchOneDueRecordAtGeneration(counter, nextRecordIndex);
    if (!applied) return false;
    lastKey = applied->key;
    lastFlags = applied->flags;
    lastPrefetched = applied;
    if (nextRecordIndex < file.records().size()) {
        lastPrefetched = &file.records()[nextRecordIndex];
        readReturnBytes = 8;
    } else {
        readReturnBytes = 0;
        firstEofObserved = true;
    }
    return true;
}
} // namespace n3d
