#include "formats/ImgArchive.hpp"
#include "formats/BinaryIO.hpp"

#include <algorithm>
#include <span>
#include <stdexcept>

namespace n3d {

ImgArchive ImgArchive::load(const std::filesystem::path& path) {
    const auto bytes = io::readFile(path);
    if (bytes.size() < img::kImageIndexRegionEnd) {
        throw std::runtime_error("IMG file is too small for both 256-entry image directories: " +
                                 path.string());
    }

    ImgArchive out;
    out.reservedDword_ = io::readU32LE(bytes, 0);
    out.firstDataOffset_ = io::readU32LE(bytes, 4);
    if (out.firstDataOffset_ < img::kImageIndexRegionEnd ||
        out.firstDataOffset_ > bytes.size()) {
        throw std::runtime_error("IMG first image offset is invalid: " + path.string());
    }

    auto readDirectory = [&](std::size_t base, std::vector<std::uint32_t>& directory) {
        directory.reserve(img::kImageIndexEntries);
        for (std::size_t i = 0; i < img::kImageIndexEntries; ++i) {
            const auto offset = io::readU32LE(bytes, base + i * sizeof(std::uint32_t));
            if (offset != 0 &&
                (offset < out.firstDataOffset_ || offset >= bytes.size())) {
                throw std::runtime_error("IMG directory entry points outside image data: " +
                                         path.string());
            }
            directory.push_back(offset);
        }
    };

    readDirectory(img::kWallImageDirectoryOffset, out.wallSlotOffsets_);
    readDirectory(img::kObjectImageDirectoryOffset, out.objectSlotOffsets_);

    if (bytes.size() >= img::kHighSequenceBankEnd) {
        out.hasSequenceDefinitions_ = true;
        for (std::size_t i = 0; i < img::kSequenceDefinitionsPerBank; ++i) {
            const auto lowOffset = img::sequenceDefinitionOffset(
                img::SequenceBank::LowSelector, static_cast<std::uint8_t>(i));
            const auto highOffset = img::sequenceDefinitionOffset(
                img::SequenceBank::HighSelector, static_cast<std::uint8_t>(i));
            std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(lowOffset),
                        img::kSequenceDefinitionBytes,
                        out.lowSequenceDefinitions_[i].raw.begin());
            std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(highOffset),
                        img::kSequenceDefinitionBytes,
                        out.highSequenceDefinitions_[i].raw.begin());
        }
    }

    std::size_t pos = out.firstDataOffset_;
    while (pos < bytes.size()) {
        if (pos + 10 > bytes.size()) {
            throw std::runtime_error("Truncated IMG frame header: " + path.string());
        }

        ImgFrame frame;
        frame.fileOffset = static_cast<std::uint32_t>(pos);
        frame.width = bytes[pos];
        frame.height = bytes[pos + 1];
        std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(pos + 2), 8, frame.metadata.begin());

        const std::size_t pixelCount = static_cast<std::size_t>(frame.width) * frame.height;
        if (pos + 10 + pixelCount > bytes.size()) {
            throw std::runtime_error("IMG frame extends past EOF: " + path.string());
        }
        frame.pixels.assign(bytes.begin() + static_cast<std::ptrdiff_t>(pos + 10),
                            bytes.begin() + static_cast<std::ptrdiff_t>(pos + 10 + pixelCount));

        const std::size_t index = out.frames_.size();
        out.exactOffsetToFrame_.emplace(frame.fileOffset, index);
        out.frames_.push_back(std::move(frame));
        pos += 10 + pixelCount;
    }

    return out;
}

const ImgSequenceDefinition* ImgArchive::sequenceDefinition(img::SequenceBank bank,
                                                            std::uint8_t selector) const {
    if (!hasSequenceDefinitions_) {
        return nullptr;
    }
    const auto index = static_cast<std::size_t>(selector);
    if (bank == img::SequenceBank::HighSelector) {
        return &highSequenceDefinitions_[index];
    }
    return &lowSequenceDefinitions_[index];
}

std::optional<std::size_t> ImgArchive::frameAtExactOffset(std::uint32_t offset) const {
    const auto it = exactOffsetToFrame_.find(offset);
    if (it == exactOffsetToFrame_.end()) {
        return std::nullopt;
    }
    return it->second;
}

} // namespace n3d
