#pragma once

#include "game/PlayerCollisionRuntime.hpp"
#include "game/ProjectileRuntime.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>

namespace nitemare3d::game {

// Win16 V1.10 only. Source: analysis/nite3w_projectile_pool_2026-09-23.md,
// FUN_1010_5466 / FUN_1010_574C. Offsets below are relative to ONE save slot.
// Byte transport is separate from load-time MAP rebinding: a plain round trip
// preserves every byte, including stale pointers, unknown fields and states.
using ProjectileBytes = std::array<std::uint8_t, kProjectileRecordStride>;
using ProjectileSaveBytes = std::array<ProjectileBytes, kProjectilePoolCapacity>;
inline constexpr std::size_t kProjectileSaveEnd = kProjectileSaveOffset + kProjectileSaveBytes;
static_assert(kProjectileSaveEnd == 0xC553);

namespace projectile_save_detail {

// Explicit little-endian values. Never reinterpret unaligned file bytes as
// packed records, and never turn persisted Win16 offsets into host pointers.
template<std::size_t Offset>
constexpr std::uint16_t read16(const ProjectileBytes& bytes) noexcept {
    static_assert(Offset + 2 <= kProjectileRecordStride);
    return static_cast<std::uint16_t>(static_cast<std::uint32_t>(bytes[Offset]) |
                                     (static_cast<std::uint32_t>(bytes[Offset + 1]) << 8));
}

template<std::size_t Offset>
constexpr std::int16_t readSigned16(const ProjectileBytes& bytes) noexcept {
    const auto raw = read16<Offset>(bytes);
    const auto value = raw < 0x8000u ? static_cast<std::int32_t>(raw)
                                   : static_cast<std::int32_t>(raw) - 0x10000;
    return static_cast<std::int16_t>(value);
}

template<std::size_t Offset>
constexpr std::uint32_t read32(const ProjectileBytes& bytes) noexcept {
    static_assert(Offset + 4 <= kProjectileRecordStride);
    return static_cast<std::uint32_t>(read16<Offset>(bytes)) |
           (static_cast<std::uint32_t>(read16<Offset + 2>(bytes)) << 16);
}

template<std::size_t Offset>
constexpr void write16(ProjectileBytes& bytes, std::uint16_t value) noexcept {
    static_assert(Offset + 2 <= kProjectileRecordStride);
    bytes[Offset] = static_cast<std::uint8_t>(value);
    bytes[Offset + 1] = static_cast<std::uint8_t>(value >> 8);
}

template<std::size_t Offset>
constexpr void write32(ProjectileBytes& bytes, std::uint32_t value) noexcept {
    static_assert(Offset + 4 <= kProjectileRecordStride);
    write16<Offset>(bytes, static_cast<std::uint16_t>(value));
    write16<Offset + 2>(bytes, static_cast<std::uint16_t>(value >> 16));
}

} // namespace projectile_save_detail

// A structural decode, NOT validation that the record is a playable projectile.
// OBJECT animation fields use the recovered generic names from ObjectSystem.hpp.
constexpr ProjectileRuntimeRecord decodeProjectileRecord(const ProjectileBytes& bytes) noexcept {
    using namespace projectile_save_detail;
    ProjectileRuntimeRecord record{};
    record.xIsMajorAxis = readSigned16<0x00>(bytes);
    record.lineError = readSigned16<0x02>(bytes);
    record.minorErrorStep = readSigned16<0x04>(bytes);
    record.majorErrorFixup = readSigned16<0x06>(bytes);
    record.stepX = readSigned16<0x08>(bytes);
    record.stepY = readSigned16<0x0A>(bytes);
    record.state = bytes[0x0C];
    record.unknown0D = bytes[0x0D];
    record.renderObject.objectId = bytes[0x0E];
    record.renderObject.variant = bytes[0x0F];
    record.renderObject.animationAux = bytes[0x10];
    record.renderObject.animationFrame = bytes[0x11];
    record.renderObject.sequenceId = bytes[0x12];
    record.renderObject.flags = bytes[0x13];
    record.renderObject.type = bytes[0x14];
    record.renderObject.guardIndex = bytes[0x15];
    record.renderObject.animationDeadline = read32<0x16>(bytes);
    record.renderObject.mapCellOffset = read16<0x1A>(bytes);
    record.renderObject.mapCellSegment = read16<0x1C>(bytes);
    record.renderObject.worldX = readSigned16<0x1E>(bytes);
    record.renderObject.worldY = readSigned16<0x20>(bytes);
    record.renderObject.renderSortA = readSigned16<0x22>(bytes);
    record.renderObject.renderSortB = readSigned16<0x24>(bytes);
    record.renderObject.projectedYBase = readSigned16<0x26>(bytes);
    record.renderObject.runtime1A = bytes[0x28];
    record.renderObject.unknown1B = bytes[0x29];
    return record;
}

constexpr ProjectileBytes encodeProjectileRecord(const ProjectileRuntimeRecord& record) noexcept {
    using namespace projectile_save_detail;
    ProjectileBytes bytes{};
    write16<0x00>(bytes, static_cast<std::uint16_t>(record.xIsMajorAxis));
    write16<0x02>(bytes, static_cast<std::uint16_t>(record.lineError));
    write16<0x04>(bytes, static_cast<std::uint16_t>(record.minorErrorStep));
    write16<0x06>(bytes, static_cast<std::uint16_t>(record.majorErrorFixup));
    write16<0x08>(bytes, static_cast<std::uint16_t>(record.stepX));
    write16<0x0A>(bytes, static_cast<std::uint16_t>(record.stepY));
    bytes[0x0C] = record.state;
    bytes[0x0D] = record.unknown0D;
    bytes[0x0E] = record.renderObject.objectId;
    bytes[0x0F] = record.renderObject.variant;
    bytes[0x10] = record.renderObject.animationAux;
    bytes[0x11] = record.renderObject.animationFrame;
    bytes[0x12] = record.renderObject.sequenceId;
    bytes[0x13] = record.renderObject.flags;
    bytes[0x14] = record.renderObject.type;
    bytes[0x15] = record.renderObject.guardIndex;
    write32<0x16>(bytes, record.renderObject.animationDeadline);
    write16<0x1A>(bytes, record.renderObject.mapCellOffset);
    write16<0x1C>(bytes, record.renderObject.mapCellSegment);
    write16<0x1E>(bytes, static_cast<std::uint16_t>(record.renderObject.worldX));
    write16<0x20>(bytes, static_cast<std::uint16_t>(record.renderObject.worldY));
    write16<0x22>(bytes, static_cast<std::uint16_t>(record.renderObject.renderSortA));
    write16<0x24>(bytes, static_cast<std::uint16_t>(record.renderObject.renderSortB));
    write16<0x26>(bytes, static_cast<std::uint16_t>(record.renderObject.projectedYBase));
    bytes[0x28] = record.renderObject.runtime1A;
    bytes[0x29] = record.renderObject.unknown1B;
    return bytes;
}

constexpr ProjectilePool decodeProjectilePool(const ProjectileSaveBytes& bytes) noexcept {
    ProjectilePool pool{};
    for (std::size_t i = 0; i < pool.size(); ++i) pool[i] = decodeProjectileRecord(bytes[i]);
    return pool;
}

constexpr ProjectileSaveBytes encodeProjectilePool(const ProjectilePool& pool) noexcept {
    ProjectileSaveBytes bytes{};
    for (std::size_t i = 0; i < pool.size(); ++i) bytes[i] = encodeProjectileRecord(pool[i]);
    return bytes;
}

// Like GuardSaveBlock, these APIs validate this block's end, not the whole save
// container. Caller selects the physical slot first. A short span throws before
// any destination byte is changed; bytes outside this 336-byte block are unused.
inline ProjectileSaveBytes readProjectileSaveBlock(std::span<const std::uint8_t> slot) {
    if (slot.size() < kProjectileSaveEnd)
        throw std::invalid_argument("USER.SAV slot ends before projectile block");
    ProjectileSaveBytes bytes{};
    for (std::size_t i = 0; i < bytes.size(); ++i)
        for (std::size_t j = 0; j < bytes[i].size(); ++j)
            bytes[i][j] = slot[kProjectileSaveOffset + i * kProjectileRecordStride + j];
    return bytes;
}

inline void writeProjectileSaveBlock(std::span<std::uint8_t> slot, const ProjectileSaveBytes& bytes) {
    if (slot.size() < kProjectileSaveEnd)
        throw std::invalid_argument("USER.SAV slot ends before projectile block");
    for (std::size_t i = 0; i < bytes.size(); ++i)
        for (std::size_t j = 0; j < bytes[i].size(); ++j)
            slot[kProjectileSaveOffset + i * kProjectileRecordStride + j] = bytes[i][j];
}

// Explicit load-time subset, NOT a complete original loader or a per-tick rule.
// Rebind all eight saved records (including free slots), as described by the
// source audit. The 8192-byte MAP cell buffer must fit within one 16-bit segment.
// mapBaseOffset is the start of that buffer, NOT the MAP archive header offset.
//
// Modern safety contract: validate EVERY coordinate/base before changing ANY
// record. Invalid coordinates in a free/unknown-state record also return false;
// raw read/write/decode still preserve those bytes. No pointer normalization,
// animation deadline adjustment, state reset or direction recalculation occurs.
[[nodiscard]] constexpr bool
tryRebindProjectileMapCells(ProjectilePool& pool, std::uint16_t mapSegment,
                           std::uint16_t mapBaseOffset = 0) noexcept {
    constexpr std::uint32_t mapBytes = 64u * 64u * 2u;
    if (static_cast<std::uint32_t>(mapBaseOffset) + mapBytes > 0x10000u) return false;
    std::array<std::uint16_t, kProjectilePoolCapacity> offsets{};
    for (std::size_t i = 0; i < pool.size(); ++i) {
        const auto& object = pool[i].renderObject;
        const auto offset = collision::mapCellByteOffset(collision::worldToTile(object.worldX),
                                                        collision::worldToTile(object.worldY));
        if (!offset) return false;
        offsets[i] = static_cast<std::uint16_t>(mapBaseOffset + *offset);
    }
    for (std::size_t i = 0; i < pool.size(); ++i) {
        pool[i].renderObject.mapCellOffset = offsets[i];
        pool[i].renderObject.mapCellSegment = mapSegment;
    }
    return true;
}

} // namespace nitemare3d::game
