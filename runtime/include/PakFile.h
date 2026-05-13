#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <fstream>
#include <cstdint>
#include <filesystem>

struct PakEntry {
    unsigned int offset;
    unsigned int size;
    std::size_t streamIndex;
};

class PakFile {
public:
    bool Load(const std::string& directoryPath);
    std::vector<uint8_t> GetData(const std::string& filename);
    bool Exists(const std::string& filename);
private:
    bool AppendPak(const std::filesystem::path& filepath);
    bool ReadDirectory(std::size_t streamIndex);
    bool HasPakExtension(const std::filesystem::path& filepath);

    std::vector<std::ifstream> pakStreams;
    std::unordered_map<std::string, PakEntry> entries;
};
