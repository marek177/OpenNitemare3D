#pragma once

#include "formats/ImgSequenceLayout.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <unordered_map>
#include <vector>

namespace n3d {

struct ImgFrame {
    std::uint32_t fileOffset{};
    std::uint8_t width{};
    std::uint8_t height{};
    std::array<std::uint8_t, 8> metadata{};
    std::vector<std::uint8_t> pixels;
};

struct ImgSequenceDefinition {
    std::array<std::uint8_t, img::kSequenceDefinitionBytes> raw{};

    std::uint16_t intervalMs() const noexcept {
        return static_cast<std::uint16_t>(raw[0]) |
               (static_cast<std::uint16_t>(raw[1]) << 8u);
    }
    std::uint8_t frameCount() const noexcept { return raw[2]; }
    std::uint8_t alternateTableFlag() const noexcept { return raw[3]; }
};

class ImgArchive {
public:
    static ImgArchive load(const std::filesystem::path& path);

    // Entry zero is the reserved zero pointer. The remaining entries are the
    // original engine's 256-byte-indexed wall-image directory.
    std::uint32_t reservedDword() const { return reservedDword_; }
    std::uint32_t firstDataOffset() const { return firstDataOffset_; }
    const std::vector<std::uint32_t>& wallSlotOffsets() const { return wallSlotOffsets_; }
    const std::vector<std::uint32_t>& objectSlotOffsets() const { return objectSlotOffsets_; }

    // Compatibility name for the original engine's primary wall-image directory.
    const std::vector<std::uint32_t>& slotOffsets() const { return wallSlotOffsets_; }

    bool hasSequenceDefinitions() const { return hasSequenceDefinitions_; }
    const ImgSequenceDefinition* sequenceDefinition(img::SequenceBank bank,
                                                    std::uint8_t selector) const;

    const std::vector<ImgFrame>& frames() const { return frames_; }
    std::optional<std::size_t> frameAtExactOffset(std::uint32_t offset) const;

private:
    std::uint32_t reservedDword_{};
    std::uint32_t firstDataOffset_{};
    std::vector<std::uint32_t> wallSlotOffsets_;
    std::vector<std::uint32_t> objectSlotOffsets_;
    std::array<ImgSequenceDefinition, img::kSequenceDefinitionsPerBank> lowSequenceDefinitions_{};
    std::array<ImgSequenceDefinition, img::kSequenceDefinitionsPerBank> highSequenceDefinitions_{};
    bool hasSequenceDefinitions_{};
    std::vector<ImgFrame> frames_;
    std::unordered_map<std::uint32_t, std::size_t> exactOffsetToFrame_;
};

} // namespace n3d
