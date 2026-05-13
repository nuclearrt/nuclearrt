#include "PakFile.h"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <cctype>

bool PakFile::Load(const std::string& directoryPath) {
	pakStreams.clear();
	entries.clear();

	const std::filesystem::path dir(directoryPath);
	std::vector<std::filesystem::path> pakPaths;
	std::error_code ec;
	if (std::filesystem::is_directory(dir, ec)) {
		for (const std::filesystem::directory_entry& ent :
			 std::filesystem::directory_iterator(dir, ec)) {
			if (ec)
				break;
			if (!ent.is_regular_file(ec))
				continue;
			const std::filesystem::path& p = ent.path();
			if (HasPakExtension(p))
				pakPaths.push_back(p);
		}
	}
	std::sort(pakPaths.begin(), pakPaths.end());

	bool loadedAnyPak = false;
	for (const std::filesystem::path& p : pakPaths) {
		if (AppendPak(p)) loadedAnyPak = true;
	}
	return loadedAnyPak;
}

bool PakFile::AppendPak(const std::filesystem::path& filepath) {
	pakStreams.emplace_back(filepath, std::ios::binary);
	std::ifstream& stream = pakStreams.back();
	if (!stream) {
		std::cerr << "Failed to open pak file: " << filepath.string() << std::endl;
		pakStreams.pop_back();
		return false;
	}

	// read header
	char magic[4];
	stream.read(magic, 4);
	if (memcmp(magic, "PACK", 4) != 0) {
		std::cerr << "Invalid pak file magic: " << std::string(magic, 4) << std::endl;
		pakStreams.pop_back();
		return false;
	}

	unsigned int dirOffset, dirSize;
	stream.read(reinterpret_cast<char*>(&dirOffset), 4);
	stream.read(reinterpret_cast<char*>(&dirSize), 4);

	stream.seekg(dirOffset);
	const std::size_t streamIndex = pakStreams.size() - 1;
	return ReadDirectory(streamIndex);
}

bool PakFile::ReadDirectory(std::size_t streamIndex) {
	std::ifstream& pakStream = pakStreams[streamIndex];

	while (pakStream.good() && !pakStream.eof()) {
		char filename[56];
		pakStream.read(filename, 56);

		if (pakStream.eof()) break;

		bool allZeros = true;
		for (int i = 0; i < 56; i++) {
			if (filename[i] != 0) {
				allZeros = false;
				break;
			}
		}
		if (allZeros) break;
		
		filename[55] = '\0';
		std::string name = std::string(filename);
		
		// trim null bytes
		const size_t nullPos = name.find('\0');
		if (nullPos != std::string::npos) {
			name = name.substr(0, nullPos);
		}
		
		if (name.empty()) break;

		unsigned int offset = 0;
		unsigned int size = 0;
		pakStream.read(reinterpret_cast<char*>(&offset), 4);
		pakStream.read(reinterpret_cast<char*>(&size), 4);

		PakEntry entry;
		entry.offset = offset;
		entry.size = size;
		entry.streamIndex = streamIndex;
		entries.try_emplace(name, entry);
	}
	
	return true;
}

bool PakFile::HasPakExtension(const std::filesystem::path& path) {
	std::string ext = path.extension().string();
	for (char& c : ext)
	{
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	}
	return ext == ".pak";
}

std::vector<uint8_t> PakFile::GetData(const std::string& filename) {
	auto it = entries.find(filename);
	if (it == entries.end()) {
		std::cerr << "File not found in PAK: " << filename << std::endl;
		return {};
	}

	const PakEntry& entry = it->second;
	if (entry.streamIndex >= pakStreams.size())
		return {};

	std::ifstream& stream = pakStreams[entry.streamIndex];
	stream.clear();
	stream.seekg(entry.offset);

	std::vector<uint8_t> data(entry.size);
	stream.read(reinterpret_cast<char*>(data.data()), entry.size);
	return data;
}

bool PakFile::Exists(const std::string& filename) {
	return entries.find(filename) != entries.end();
}