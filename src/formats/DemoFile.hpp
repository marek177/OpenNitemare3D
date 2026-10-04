#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace n3d {

struct DemoRecord {
    std::uint8_t eventByte{};
    std::uint16_t inputMask{};
    std::uint8_t paddingByte{};
    std::uint32_t renderGeneration{}; // render-generation index, not milliseconds
};

class DemoFile {
public:
    static DemoFile load(const std::filesystem::path& path);

    const std::array<std::uint16_t, 3>& rawHeaderWords() const { return rawHeaderWords_; }
    const std::vector<DemoRecord>& records() const { return records_; }
    bool renderGenerationsAreMonotonic() const { return renderGenerationsAreMonotonic_; }

    // The original dispatcher consumes at most one due record per call. This
    // reconstruction also bounds-checks EOF; the original inspected branch does not.
    const DemoRecord* dispatchOneDueRecordAtGeneration(
        std::uint32_t currentRenderGeneration,
        std::size_t& nextRecordIndex) const noexcept;

private:
    std::array<std::uint16_t, 3> rawHeaderWords_{};
    std::vector<DemoRecord> records_;
    bool renderGenerationsAreMonotonic_{true};
};

} // namespace n3d
