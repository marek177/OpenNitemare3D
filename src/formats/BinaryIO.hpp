#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace n3d::io {

std::vector<std::uint8_t> readFile(const std::filesystem::path& path);

std::uint16_t readU16LE(std::span<const std::uint8_t> bytes, std::size_t offset);
std::uint32_t readU32LE(std::span<const std::uint8_t> bytes, std::size_t offset);

void requireRange(std::span<const std::uint8_t> bytes,
                  std::size_t offset,
                  std::size_t length,
                  const std::string& what);

} // namespace n3d::io
