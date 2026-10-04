#include "formats/DefinitionTable.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace n3d {

DefinitionTable DefinitionTable::load(const std::filesystem::path& path) {
    std::ifstream f(path);
    if (!f) {
        throw std::runtime_error("Unable to open definitions: " + path.string());
    }

    DefinitionTable table;
    std::string line;
    std::size_t lineNo = 0;
    while (std::getline(f, line)) {
        ++lineNo;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }

        std::istringstream ss(line);
        std::string idHex;
        DefinitionRecord rec;
        if (!(ss >> idHex >> rec.visualCode >> rec.imageName >> rec.className)) {
            throw std::runtime_error("Malformed definition line " + std::to_string(lineNo) +
                                     " in " + path.string());
        }

        try {
            rec.id = static_cast<std::uint16_t>(std::stoul(idHex, nullptr, 16));
        } catch (...) {
            throw std::runtime_error("Invalid hexadecimal ID on line " + std::to_string(lineNo) +
                                     " in " + path.string());
        }

        std::getline(ss, rec.description);
        while (!rec.description.empty() && (rec.description.front() == ' ' || rec.description.front() == '\t')) {
            rec.description.erase(rec.description.begin());
        }
        table.records_.push_back(std::move(rec));
    }
    return table;
}

const DefinitionRecord* DefinitionTable::find(std::uint16_t id) const {
    for (const auto& rec : records_) {
        if (rec.id == id) {
            return &rec;
        }
    }
    return nullptr;
}

bool DefinitionTable::isClass(std::uint16_t id, std::string_view className) const {
    const auto* rec = find(id);
    return rec != nullptr && rec->className == className;
}

} // namespace n3d
