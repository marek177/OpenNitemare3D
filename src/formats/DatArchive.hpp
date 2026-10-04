#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace n3d {

struct DatEntry {
    std::uint16_t length{};
    std::uint32_t offset{};
    std::vector<std::uint8_t> data;

    std::string typeGuess() const;
};

class DatArchive {
public:
    static DatArchive load(const std::filesystem::path& path);

    const std::vector<DatEntry>& entries() const { return entries_; }
    std::size_t headerBytesUsed() const { return headerBytesUsed_; }
    std::size_t firstPayloadOffset() const { return firstPayloadOffset_; }

private:
    std::vector<DatEntry> entries_;
    std::size_t headerBytesUsed_{};
    std::size_t firstPayloadOffset_{};
};

} // namespace n3d
