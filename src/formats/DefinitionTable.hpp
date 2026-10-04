#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace n3d {

struct DefinitionRecord {
    std::uint16_t id{};
    std::string visualCode;
    std::string imageName;
    std::string className;
    std::string description;
};

class DefinitionTable {
public:
    static DefinitionTable load(const std::filesystem::path& path);
    const std::vector<DefinitionRecord>& records() const { return records_; }

    const DefinitionRecord* find(std::uint16_t id) const;
    bool isClass(std::uint16_t id, std::string_view className) const;

private:
    std::vector<DefinitionRecord> records_;
};

} // namespace n3d
