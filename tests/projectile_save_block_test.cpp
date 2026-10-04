#include "game/ProjectileSaveBlock.hpp"
#include "game/PlayerRuntime.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <stdexcept>
#include <vector>

namespace {
using namespace nitemare3d::game;

struct Checks {
    std::size_t count{}, failures{};
    void require(bool ok, const char* message) {
        ++count;
        if (!ok) {
            if (failures < 8) std::cerr << "FAIL: " << message << '\n';
            ++failures;
        }
    }
    template<class Function>
    void rejectsShortInput(Function action) {
        bool expectedException = false;
        try { action(); }
        catch (const std::invalid_argument&) { expectedException = true; }
        catch (...) {}
        require(expectedException, "short input must throw invalid_argument");
    }
};

ProjectileSaveBytes pattern(std::uint32_t seed) {
    ProjectileSaveBytes bytes{};
    for (auto& record : bytes) {
        for (auto& byte : record) {
            seed = seed * 1664525u + 1013904223u;
            byte = static_cast<std::uint8_t>(seed >> 24);
        }
    }
    return bytes;
}

void checkFieldMapping(Checks& c) {
    // A literal, independent mapping oracle: round-trip alone would not detect
    // a symmetric encoder/decoder that accidentally swaps the same two fields.
    ProjectileBytes bytes{};
    for (std::size_t i = 0; i < bytes.size(); ++i) bytes[i] = static_cast<std::uint8_t>(i);
    const auto r = decodeProjectileRecord(bytes);
    c.require(r.xIsMajorAxis == 0x0100, "axis offset");
    c.require(r.lineError == 0x0302, "error offset");
    c.require(r.minorErrorStep == 0x0504, "minor error offset");
    c.require(r.majorErrorFixup == 0x0706, "major error offset");
    c.require(r.stepX == 0x0908 && r.stepY == 0x0B0A, "movement sign offsets");
    c.require(r.state == 0x0C && r.unknown0D == 0x0D, "state and unknown byte");
    c.require(r.renderObject.objectId == 0x0E && r.renderObject.variant == 0x0F, "object template bytes");
    c.require(r.renderObject.animationAux == 0x10 && r.renderObject.animationFrame == 0x11, "animation bytes");
    c.require(r.renderObject.sequenceId == 0x12 && r.renderObject.flags == 0x13, "sequence and flags");
    c.require(r.renderObject.type == 0x14 && r.renderObject.guardIndex == 0x15, "object type and auxiliary byte");
    c.require(r.renderObject.animationDeadline == 0x19181716u, "32-bit deadline little-endian");
    c.require(r.renderObject.mapCellOffset == 0x1B1A && r.renderObject.mapCellSegment == 0x1D1C, "far pointer order");
    c.require(r.renderObject.worldX == 0x1F1E && r.renderObject.worldY == 0x2120, "world coordinate offsets");
    c.require(r.renderObject.renderSortA == 0x2322 && r.renderObject.renderSortB == 0x2524, "render cache offsets");
    c.require(r.renderObject.projectedYBase == 0x2726, "projection baseline offset");
    c.require(r.renderObject.runtime1A == 0x28 && r.renderObject.unknown1B == 0x29, "final bytes");
    c.require(encodeProjectileRecord(r) == bytes, "known fixture exact round trip");

    // Each byte position takes every possible value, without erasing its neighbors.
    const auto original = bytes;
    for (std::size_t pos = 0; pos < bytes.size(); ++pos) {
        for (unsigned value = 0; value < 256; ++value) {
            bytes = original;
            bytes[pos] = static_cast<std::uint8_t>(value);
            c.require(encodeProjectileRecord(decodeProjectileRecord(bytes)) == bytes,
                      "every byte, including unknown fields, is preserved");
        }
    }
    // Exhaust signed 16-bit storage across all signed members, including INT16_MIN.
    constexpr std::array<std::size_t, 11> signedOffsets{0, 2, 4, 6, 8, 10, 30, 32, 34, 36, 38};
    for (std::uint32_t raw = 0; raw < 65536u; ++raw) {
        bytes = original;
        for (const auto offset : signedOffsets) {
            bytes[offset] = static_cast<std::uint8_t>(raw);
            bytes[offset + 1] = static_cast<std::uint8_t>(raw >> 8);
        }
        const auto record = decodeProjectileRecord(bytes);
        const auto expected = raw <= 32767u ? static_cast<std::int32_t>(raw)
                                           : static_cast<std::int32_t>(raw) - 65536;
        const std::array<std::int16_t, 11> actual{
            record.xIsMajorAxis, record.lineError, record.minorErrorStep,
            record.majorErrorFixup, record.stepX, record.stepY,
            record.renderObject.worldX, record.renderObject.worldY,
            record.renderObject.renderSortA, record.renderObject.renderSortB,
            record.renderObject.projectedYBase
        };
        for (const auto value : actual) c.require(value == expected, "signed word decode");
        c.require(encodeProjectileRecord(record) == bytes, "signed word encode");
    }
    for (std::uint32_t seed = 0; seed < 512; ++seed) {
        const auto raw = pattern(seed);
        c.require(encodeProjectilePool(decodeProjectilePool(raw)) == raw, "full eight-slot round trip");
    }
    for (const auto deadline : {0u, 0x7FFFFFFFu, 0x80000000u, 0xFFFFFFFFu}) {
        ProjectileRuntimeRecord record{};
        record.renderObject.animationDeadline = deadline;
        const auto encoded = encodeProjectileRecord(record);
        c.require(encoded[0x16] == (deadline & 0xFFu) &&
                  encoded[0x19] == (deadline >> 24), "unsigned deadline byte order");
        c.require(decodeProjectileRecord(encoded).renderObject.animationDeadline == deadline,
                  "deadline retains all 32 bits");
    }
}

void checkTransport(Checks& c) {
    // This is a synthetic slot buffer, not an original USER.SAV sample.
    constexpr std::size_t slotBytes = 0xD6E7;
    auto raw = pattern(13);
    std::vector<std::uint8_t> shortBuffer(kProjectileSaveEnd, 0xA5);
    const auto before = shortBuffer;
    for (std::size_t size = 0; size < kProjectileSaveEnd; ++size) {
        const auto prefix = std::span<std::uint8_t>(shortBuffer).first(size);
        c.rejectsShortInput([&] { (void)readProjectileSaveBlock(prefix); });
        c.rejectsShortInput([&] { writeProjectileSaveBlock(prefix, raw); });
    }
    c.require(shortBuffer == before, "all rejected writes are non-mutating");
    writeProjectileSaveBlock(shortBuffer, raw);
    c.require(readProjectileSaveBlock(shortBuffer) == raw, "minimum sufficient prefix accepted");

    // Shifted/unaligned slot views, and a second physical slot selected by caller.
    for (const auto start : {std::size_t{1}, std::size_t{1 + slotBytes}}) {
        std::vector<std::uint8_t> file(1 + 2 * slotBytes + 17, 0x5A);
        auto slot = std::span<std::uint8_t>(file).subspan(start, slotBytes);
        writeProjectileSaveBlock(slot, raw);
        c.require(readProjectileSaveBlock(slot) == raw, "selected slot exact read/write");
        for (std::size_t pos = 0; pos < file.size(); ++pos) {
            const auto begin = start + kProjectileSaveOffset;
            if (pos >= begin && pos < begin + kProjectileSaveBytes) {
                const auto relative = pos - begin;
                c.require(file[pos] == raw[relative / 42][relative % 42], "pool slot order");
            } else {
                c.require(file[pos] == 0x5A, "neighboring blocks and slots untouched");
            }
        }
        const auto fullSnapshot = file;
        const auto pool = decodeProjectilePool(readProjectileSaveBlock(slot));
        c.require(file == fullSnapshot, "read/decode has no input side effects");
        writeProjectileSaveBlock(slot, encodeProjectilePool(pool));
        c.require(file == fullSnapshot, "load/store without rebinding is byte-identical");
    }
}

ProjectilePool validPool() {
    auto pool = decodeProjectilePool(pattern(27));
    for (std::size_t i = 0; i < pool.size(); ++i) {
        pool[i].state = static_cast<std::uint8_t>(i); // includes free and unknown states
        pool[i].renderObject.worldX = static_cast<std::int16_t>(i * 64 + 32);
        pool[i].renderObject.worldY = static_cast<std::int16_t>((63 - i) * 64 + 32);
    }
    return pool;
}

void checkRebind(Checks& c) {
    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 64; ++x) {
            auto pool = validPool();
            for (auto& r : pool) {
                r.renderObject.worldX = static_cast<std::int16_t>(x * 64 + 63);
                r.renderObject.worldY = static_cast<std::int16_t>(y * 64);
            }
            const auto before = encodeProjectilePool(pool);
            c.require(tryRebindProjectileMapCells(pool, 0xBEEF, 0x120), "valid rebind");
            const auto after = encodeProjectilePool(pool);
            for (std::size_t i = 0; i < pool.size(); ++i) {
                const auto offset = 0x120 + y * 128 + x * 2;
                c.require(pool[i].renderObject.mapCellOffset == offset, "MAP row-major offset with base");
                c.require(pool[i].renderObject.mapCellSegment == 0xBEEF, "loaded segment assigned");
                for (std::size_t byte = 0; byte < 42; ++byte) {
                    if (byte < 0x1A || byte > 0x1D)
                        c.require(after[i][byte] == before[i][byte], "rebind changes no state/deadline/cache bytes");
                }
            }
        }
    }
    for (std::size_t badSlot = 0; badSlot < 8; ++badSlot) {
        for (const int bad : {-32768, -64, -1, 4096, 32767}) {
            for (int axis = 0; axis < 2; ++axis) {
                auto pool = validPool();
                if (axis == 0) pool[badSlot].renderObject.worldX = static_cast<std::int16_t>(bad);
                else pool[badSlot].renderObject.worldY = static_cast<std::int16_t>(bad);
                const auto before = encodeProjectilePool(pool);
                c.require(!tryRebindProjectileMapCells(pool, 0x1234), "invalid coordinate rejected, even free slot");
                c.require(encodeProjectilePool(pool) == before, "failed rebind has no partial changes");
            }
        }
    }
    // Every base offset, including the last legal 8-KiB buffer and wrap boundary.
    for (std::uint32_t base = 0; base < 65536u; ++base) {
        auto pool = validPool();
        pool[7].renderObject.worldX = 4095;
        pool[7].renderObject.worldY = 4095;
        const auto before = encodeProjectilePool(pool);
        const bool result = tryRebindProjectileMapCells(pool, 0xFFFF, static_cast<std::uint16_t>(base));
        c.require(result == (base <= 57344u), "segment buffer fit contract");
        if (result) {
            c.require(pool[7].renderObject.mapCellOffset == base + 8190u, "last cell no 16-bit overflow");
            const auto once = encodeProjectilePool(pool);
            c.require(tryRebindProjectileMapCells(pool, 0xFFFF, static_cast<std::uint16_t>(base)) &&
                      encodeProjectilePool(pool) == once, "rebind idempotent");
        } else {
            c.require(encodeProjectilePool(pool) == before, "invalid base leaves all records unchanged");
        }
    }
}
} // namespace

int main() {
    static_assert(kProjectilePoolCapacity == 8 && kProjectileRecordStride == 42);
    static_assert(kProjectileSaveOffset == 0xC403 && kProjectileSaveEnd == 0xC553);
    static_assert(encodeProjectileRecord(decodeProjectileRecord(ProjectileBytes{})) == ProjectileBytes{});
    Checks checks;
    checkFieldMapping(checks);
    checkTransport(checks);
    checkRebind(checks);
    std::cout << "projectile_save_block_test: " << checks.count << " checks, "
              << checks.failures << " failures\n";
    return checks.failures ? 1 : 0;
}
