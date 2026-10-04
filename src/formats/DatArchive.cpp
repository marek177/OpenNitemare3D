#include "formats/DatArchive.hpp"
#include "formats/BinaryIO.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <span>
#include <stdexcept>

namespace n3d {

std::string DatEntry::typeGuess() const {
    if (data.size() >= 4 && data[0] == 'M' && data[1] == 'T' && data[2] == 'h' && data[3] == 'd') {
        return "MIDI";
    }
    static constexpr char vocSig[] = "Creative Voice File";
    if (data.size() >= sizeof(vocSig) - 1 &&
        std::equal(std::begin(vocSig), std::end(vocSig) - 1, data.begin())) {
        return "VOC";
    }
    if (data.size() >= 4 && data[0] == 0x0A) {
        return "PCX-or-PCX-like";
    }
    if (data.size() >= 3 && data[0] == 'I' && data[1] == 'B' && data[2] == 'K') {
        return "IBK-instrument-bank";
    }
    if (data.empty()) {
        return "empty";
    }
    return "raw/PCM/unknown";
}

DatArchive DatArchive::load(const std::filesystem::path& path) {
    const auto bytes = io::readFile(path);
    if (bytes.size() < 6) {
        throw std::runtime_error("DAT archive too small: " + path.string());
    }

    DatArchive result;
    std::size_t pos = 0;
    std::size_t firstPayload = std::numeric_limits<std::size_t>::max();
    bool foundTerminalEntry = false;

    // The archive index consists of 6-byte records:
    // uint16 length, uint32 absolute file offset. The final descriptor is the
    // one whose offset + length reaches EOF. Files observed here contain
    // padding between the last descriptor and the first payload.
    while (pos + 6 <= bytes.size()) {
        const auto length = io::readU16LE(bytes, pos);
        const auto offset = io::readU32LE(bytes, pos + 2);
        pos += 6;

        if (offset > bytes.size() || static_cast<std::size_t>(length) > bytes.size() - offset) {
            throw std::runtime_error("Invalid DAT index entry in " + path.string());
        }

        DatEntry entry;
        entry.length = length;
        entry.offset = offset;
        entry.data.assign(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                          bytes.begin() + static_cast<std::ptrdiff_t>(offset + length));
        result.entries_.push_back(std::move(entry));
        if (offset != 0) {
            firstPayload = std::min(firstPayload, static_cast<std::size_t>(offset));
        }

        if (static_cast<std::size_t>(offset) + length == bytes.size()) {
            foundTerminalEntry = true;
            break;
        }
    }

    if (!foundTerminalEntry) {
        throw std::runtime_error("DAT terminal descriptor not found: " + path.string());
    }

    result.headerBytesUsed_ = pos;
    result.firstPayloadOffset_ = firstPayload;
    return result;
}

} // namespace n3d
