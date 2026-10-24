#pragma once
// Evidence-safe byte comparison and reversible snapshot storage.
// Does not claim knowledge of any unverified USER.SAV field layout.
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>
namespace n3d::parity {
struct Difference { std::size_t offset; std::uint8_t expected; std::uint8_t actual; };
inline std::optional<Difference> first_difference(std::span<const std::uint8_t> a,
                                                    std::span<const std::uint8_t> b) {
    const auto n = a.size() < b.size() ? a.size() : b.size();
    for (std::size_t i=0; i<n; ++i) if (a[i]!=b[i]) return Difference{i,a[i],b[i]};
    return std::nullopt;
}
struct CompareResult {
    bool equal;
    std::size_t expected_size;
    std::size_t actual_size;
    std::optional<Difference> first_byte_difference;
};
inline CompareResult compare(std::span<const std::uint8_t> a,
                             std::span<const std::uint8_t> b) {
    return {a.size()==b.size() && !first_difference(a,b).has_value(),
            a.size(),b.size(),first_difference(a,b)};
}
class LosslessSnapshot {
    std::vector<std::uint8_t> raw_;
public:
    explicit LosslessSnapshot(std::span<const std::uint8_t> bytes) : raw_(bytes.begin(),bytes.end()) {}
    std::span<const std::uint8_t> bytes() const noexcept { return raw_; }
    std::size_t size() const noexcept { return raw_.size(); }
    std::uint8_t at(std::size_t offset) const { return raw_.at(offset); }
    void patch(std::size_t offset, std::uint8_t value) { raw_.at(offset)=value; }
    std::vector<std::uint8_t> serialize() const { return raw_; }
};
inline bool exact_indexed_frame_and_dac(std::span<const std::uint8_t> frame_expected,
                                        std::span<const std::uint8_t> dac_expected,
                                        std::span<const std::uint8_t> frame_actual,
                                        std::span<const std::uint8_t> dac_actual) {
    constexpr std::size_t indexed_pixels=320u*200u, dac_bytes=256u*3u;
    return frame_expected.size()==indexed_pixels && frame_actual.size()==indexed_pixels &&
           dac_expected.size()==dac_bytes && dac_actual.size()==dac_bytes &&
           compare(frame_expected,frame_actual).equal && compare(dac_expected,dac_actual).equal;
}
} // namespace n3d::parity
