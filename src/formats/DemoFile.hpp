#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>
namespace n3d {
struct DemoRecord {
    std::uint16_t key{};
    std::uint16_t flags{};
    std::uint32_t renderGeneration{};
};
class DemoFile {
public:
    static DemoFile load(const std::filesystem::path& path);
    const std::array<std::uint16_t, 3>& rawHeaderWords() const { return rawHeaderWords_; }
    const std::vector<DemoRecord>& records() const { return records_; }
    bool renderGenerationsAreMonotonic() const { return renderGenerationsAreMonotonic_; }
    // Bounded clean-room scheduler primitive, NOT a complete DOS scheduler.
    const DemoRecord* dispatchOneDueRecordAtGeneration(
        std::uint32_t currentRenderGeneration, std::size_t& nextRecordIndex) const noexcept;
private:
    std::array<std::uint16_t, 3> rawHeaderWords_{};
    std::vector<DemoRecord> records_;
    bool renderGenerationsAreMonotonic_{true};
};
// The observed original reader retains last key/flags and prefetch on EOF;
// the reader's outer caller and subsequent interrupts are not modeled.
struct DemoReaderState {
    std::size_t nextRecordIndex = 0;
    std::uint16_t lastKey = 0;
    std::uint16_t lastFlags = 0;
    const DemoRecord* lastPrefetched = nullptr;
    bool firstEofObserved = false;
    // true: a due record was applied, readReturnBytes 8 or 0;
    // false: no record was due (or all records were consumed).
    bool applyDue(const DemoFile& file, std::uint32_t counter,
                  std::uint16_t& readReturnBytes) noexcept;
};
} // namespace n3d
