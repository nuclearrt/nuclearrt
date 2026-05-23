#pragma once

#include "Application.h"
#include "Extension.h"
#include "ObjectInstance.h"
#include <string>
#include <filesystem>
#include <sstream>
#include "mini/ini.h"

class INIInstance
{
public:
	bool dirty = false;
	bool ReadOnly = false;
	bool Encrypted = false;
	bool Compressed = false;
	bool CaseSensitive = false;
	bool UseQuotes = false;

	std::string EncryptionKey = "";
	std::string NewLineText = "\n";

	int UndoBufferSize;
	int RedoBufferSize;

	std::string FilePath;

private:
	std::unique_ptr<mINI::INIFile> iniFile;
	mINI::INIStructure ini;

public:

	void ReadIniData(std::string fileData) {
		ini.clear();

		//mini doesn't have a built-in function to read from a string

		if (fileData.empty()) {
			dirty = false;
			return;
		}

		if (fileData.size() >= 3 &&
			static_cast<unsigned char>(fileData[0]) == 0xEF &&
			static_cast<unsigned char>(fileData[1]) == 0xBB &&
			static_cast<unsigned char>(fileData[2]) == 0xBF)
		{
			fileData.erase(0, 3);
		}

		std::string section;
		bool inSection = false;
		mINI::INIParser::T_ParseValues parseData;
		std::istringstream input(fileData);
		std::string line;

		while (std::getline(input, line))
		{
			if (!line.empty() && line.back() == '\r') {
				line.pop_back();
			}

			auto parseResult = mINI::INIParser::parseLine(line, parseData);
			if (parseResult == mINI::INIParser::PDataType::PDATA_SECTION)
			{
				inSection = true;
				section = parseData.first;
				ini[section];
			}
			else if (inSection && parseResult == mINI::INIParser::PDataType::PDATA_KEYVALUE)
			{
				ini[section][parseData.first] = parseData.second;
			}
		}

		dirty = false;
	}

	void LoadIniFromPath(std::string filePath)
	{
		FilePath = std::move(filePath);
		iniFile = std::make_unique<mINI::INIFile>(FilePath);
		ini.clear();
		if (iniFile) {
			iniFile->read(ini);
		}
		dirty = false;
	}

	void SaveINI()
	{
		if (iniFile && !ReadOnly && FilePath != "") {
			iniFile->write(ini);
			dirty = false;
		}
	}

	bool GroupExists(const std::string& group) const
	{
		return ini.has(group);
	}

	bool GroupItemExists(const std::string& group, const std::string& item) const
	{
		return ini.has(group) && ini.get(group).has(item);
	}

	std::string GetValue(const std::string& group, const std::string& item, const std::string& defaultValue = "")
	{
		if (ini.has(group) && ini[group].has(item)) {
			return ini[group][item];
		}
		return defaultValue;
	}

	void SetValue(const std::string& group, const std::string& item, const std::string& value)
	{
		if (ReadOnly) return;

		ini[group][item] = value;
		dirty = true;
	}

	void DeleteGroup(const std::string& group)
	{
		if (ReadOnly) return;

		if (ini.has(group))
		{
			ini.remove(group);
			dirty = true;
		}
	}

	void DeleteGroupItem(const std::string& group, const std::string& item)
	{
		if (ReadOnly) return;

		if (ini.has(group) && ini[group].has(item))
		{
			ini[group].remove(item);
			dirty = true;
		}
	}

	void Clear()
	{
		if (ReadOnly) return;

		ini.clear();
		dirty = true;
	}
};

class IniPlusPlusExtension : public Extension
{
public:
	IniPlusPlusExtension(unsigned int objectInfoHandle, int type, std::string name,
		bool defaultFile = true, bool readOnly = false, std::string defaultFilePath = "",
		int8_t baseFolder = 2, std::string initialData = "", bool createFolders = true,
		bool enableAutoSave = true, bool useStandardSettings = true,
		bool useCompression = false, bool useEncryption = false, std::string encryptionKey = "",
		bool newLine = false, std::string newLineText = "", bool alwaysQuoteStrings = false,
		int8_t repeatedGroups = 3, int8_t repeatedItems = 1, int8_t undoBufferSize = 0, int8_t redoBufferSize = 0,
		bool caseSensitive = false, bool saveRepeatedItems = false,
		bool escapeCharsInGroupNames = false, bool escapeCharsInItemNames = false,
		bool escapeCharsInItemValues = false, bool globalData = false,
		bool index1Based = false, bool enableAutoLoad = false, bool loadAndSaveSubGroups = false,
		bool allowEmptyGroups = true, std::string globalDataKey = "")
		: Extension(objectInfoHandle, type, name), DefaultFile(defaultFile), ReadOnly(readOnly), DefaultFilePath(defaultFilePath),
		BaseFolder(baseFolder), InitialData(initialData), CreateFolders(createFolders), EnableAutoSave(enableAutoSave), UseStandardSettings(useStandardSettings),
		UseCompression(useCompression), UseEncryption(useEncryption), EncryptionKey(encryptionKey), NewLine(newLine), NewLineText(newLineText), AlwaysQuoteStrings(alwaysQuoteStrings),
		RepeatedGroups(repeatedGroups), RepeatedItems(repeatedItems), UndoBufferSize(undoBufferSize), RedoBufferSize(redoBufferSize), CaseSensitive(caseSensitive), SaveRepeatedItems(saveRepeatedItems),
		EscapeCharsInGroupNames(escapeCharsInGroupNames), EscapeCharsInItemNames(escapeCharsInItemNames), EscapeCharsInItemValues(escapeCharsInItemValues), GlobalData(globalData),
		Index1Based(index1Based), EnableAutoLoad(enableAutoLoad), LoadAndSaveSubGroups(loadAndSaveSubGroups), AllowEmptyGroups(allowEmptyGroups), GlobalDataKey(globalDataKey)
	{}

	void Initialize() override;
	void Update(float deltaTime) override;

	void SetCurrentGroup(const std::string &group);

	void SetCurrentGroupItemValue(const std::string &item, int type, double value);
	void SetCurrentGroupItemString(const std::string &item, const std::string &value);

	void SetItemValue(const std::string &group, const std::string &item, int type, double value);
	void SetItemString(const std::string &group, const std::string &item, const std::string &value);

	void DeleteCurrentGroup();
	void DeleteGroup(const std::string &group);

	void DeleteCurrentGroupItem(const std::string &item);
	void DeleteGroupItem(const std::string &group, const std::string &item);

	void ClearINI();

	void Save();

	bool CurrentGroupExists() const;
	bool GroupExists(const std::string &group) const;

	bool CurrentGroupItemExists(const std::string &item) const;
	bool GroupItemExists(const std::string &group, const std::string &item) const;

	int GetCurrentGroupItemValue(const std::string &item, int defaultValue);
	std::string GetCurrentGroupItemString(const std::string &item, const std::string &defaultValue);

	int GetItemValue(const std::string &group, const std::string &item, int defaultValue = 0);
	std::string GetItemString(const std::string &group, const std::string &item, const std::string &defaultValue);
private:
	std::string CurrentGroup;

	bool DefaultFile = true;
	bool ReadOnly = false;
	int8_t BaseFolder = 2;
	bool CreateFolders = true;
	bool EnableAutoSave = true;
	bool UseStandardSettings = true;
	bool UseCompression = false;
	bool UseEncryption = false;
	bool NewLine = false;
	bool AlwaysQuoteStrings = false;
	int8_t RepeatedGroups = 3;
	int8_t RepeatedItems = 1;
	int8_t UndoBufferSize = 0;
	int8_t RedoBufferSize = 0;
	bool CaseSensitive = false;
	bool SaveRepeatedItems = false;
	bool EscapeCharsInGroupNames = false;
	bool EscapeCharsInItemNames = false;
	bool EscapeCharsInItemValues = false;
	bool GlobalData = false;
	bool Index1Based = false;
	bool EnableAutoLoad = false;
	bool LoadAndSaveSubGroups = false;
	bool AllowEmptyGroups = true;
	std::string DefaultFilePath = "";
	std::string InitialData = "";
	std::string EncryptionKey = "";
	std::string NewLineText = "";
	std::string GlobalDataKey = "";

	static std::unordered_map<std::string, INIInstance*> globalDataMap;
	INIInstance *currentData;

	std::filesystem::path GetBaseSaveDirectory(int8_t baseFolder = 2, const std::string &defaultFilePath = "");
};