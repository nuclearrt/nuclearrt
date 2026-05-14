#pragma once

#include <cstdint>

#include "Application.h"
#include "Extension.h"
#include "ObjectInstance.h"

struct StoreData {
	uint32_t integerCount;
	uint8_t integerBase;
	std::vector<int> integers;

	uint32_t boolCount;
	uint8_t boolBase;
	std::vector<bool> bools;

	uint32_t shortCount;
	uint8_t shortBase;
	std::vector<short> shorts;

	uint32_t stringCount;
	uint8_t stringBase;
	std::vector<std::string> strings;
};

class GlobalStoreXExtension : public Extension {
public:
	GlobalStoreXExtension(unsigned int objectInfoHandle, int type, std::string name, int integerCount, int stringCount, int boolCount, int shortCount, unsigned char flags)
		: Extension(objectInfoHandle, type, name) {
		if (!initialized) {
			GlobalStoreXExtension::data.integerCount = integerCount;
			GlobalStoreXExtension::data.stringCount = stringCount;
			GlobalStoreXExtension::data.boolCount = boolCount;
			GlobalStoreXExtension::data.shortCount = shortCount;
			GlobalStoreXExtension::data.integerBase = flags & 0x01 ? 0 : 1;
			GlobalStoreXExtension::data.stringBase = flags & 0x02 ? 0 : 1;
			GlobalStoreXExtension::data.boolBase = flags & 0x04 ? 0 : 1;
			GlobalStoreXExtension::data.shortBase = flags & 0x08 ? 0 : 1;
		}
	}

	void Initialize() override;

	void SetInteger(int index, int value);
	void SetString(int index, const std::string& value);
	void SetBool(int index, int value);
	void SetShort(int index, short value);

	void AddInteger(int index, int value);
	void AddShort(int index, short value);
	void AddString(int index, const std::string& value);

	void SubtractInteger(int index, int value);
	void SubtractShort(int index, short value);

	void SetRangeInteger(int startIndex, int endIndex, int value);
	void SetRangeString(int startIndex, int endIndex, const std::string& value);
	void SetRangeBool(int startIndex, int endIndex, int value);
	void SetRangeShort(int startIndex, int endIndex, short value);

	void ClearIntegers();
	void ClearStrings();
	void ClearBools();
	void ClearShorts();

	void ResizeIntegers(int size);
	void ResizeStrings(int size);
	void ResizeBools(int size);
	void ResizeShorts(int size);

	void ExpandIntegers(int size);
	void ExpandStrings(int size);
	void ExpandBools(int size);
	void ExpandShorts(int size);

	int GetInteger(int index);
	std::string GetString(int index);
	bool GetBool(int index);
	short GetShort(int index);

	unsigned char GetIntegerBase() const { return GlobalStoreXExtension::data.integerBase; }
	unsigned char GetStringBase() const { return GlobalStoreXExtension::data.stringBase; }
	unsigned char GetBoolBase() const { return GlobalStoreXExtension::data.boolBase; }
	unsigned char GetShortBase() const { return GlobalStoreXExtension::data.shortBase; }

	int GetIntegerArraySize() const { return GlobalStoreXExtension::data.integerCount; }
	int GetStringArraySize() const { return GlobalStoreXExtension::data.stringCount; }
	int GetBoolArraySize() const { return GlobalStoreXExtension::data.boolCount; }
	int GetShortArraySize() const { return GlobalStoreXExtension::data.shortCount; }

	void SaveAllINI(const std::string& fileName, const std::string& group);
	void LoadAllINI(const std::string& fileName, const std::string& group);

	void SaveIntegerINI(const std::string& fileName, const std::string& group);
	void LoadIntegerINI(const std::string& fileName, const std::string& group);
	void SaveStringINI(const std::string& fileName, const std::string& group);
	void LoadStringINI(const std::string& fileName, const std::string& group);
	void SaveBoolINI(const std::string& fileName, const std::string& group);
	void LoadBoolINI(const std::string& fileName, const std::string& group);
	void SaveShortINI(const std::string& fileName); // for some reason shorts can't be saved to a specific group with ini - shishkabob
	void LoadShortINI(const std::string& fileName);

	void SaveAllBinary(const std::string& fileName);
	void LoadAllBinary(const std::string& fileName);

	void SaveIntegerBinary(const std::string& fileName);
	void LoadIntegerBinary(const std::string& fileName);
	void SaveStringBinary(const std::string& fileName);
	void LoadStringBinary(const std::string& fileName);
	void SaveBoolBinary(const std::string& fileName);
	void LoadBoolBinary(const std::string& fileName);
	void SaveShortBinary(const std::string& fileName);
	void LoadShortBinary(const std::string& fileName);

private:
	static std::string GetNormalizedPath(const std::string& path);
	static StoreData data;

	static bool initialized;
};
