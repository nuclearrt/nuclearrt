#include "GlobalStoreXExtension.h"

#include <filesystem>
#include <iostream>

#include "BinaryTools/BinaryReader.h"
#include "BinaryTools/BinaryWriter.h"
#include "mini/ini.h"

bool GlobalStoreXExtension::initialized = false;
StoreData GlobalStoreXExtension::data;

void GlobalStoreXExtension::Initialize()
{
    if (initialized) return;

    data.integers.resize(data.integerCount);
    data.strings.resize(data.stringCount);
    data.bools.resize(data.boolCount);
    data.shorts.resize(data.shortCount);

    initialized = true;
}

void GlobalStoreXExtension::SetInteger(int index, int value)
{
    index -= data.integerBase;
    if (index < 0 || index >= data.integers.size()) return;

    data.integers[index] = value;
}

void GlobalStoreXExtension::SetString(int index, const std::string& value)
{
    index -= data.stringBase;
    if (index < 0 || index >= data.strings.size()) return;

    data.strings[index] = value;
}

void GlobalStoreXExtension::SetBool(int index, int value)
{
    index -= data.boolBase;
    if (index < 0 || index >= data.bools.size()) return;

    data.bools[index] = value != 0;
}

void GlobalStoreXExtension::SetShort(int index, short value)
{
    index -= data.shortBase;
    if (index < 0 || index >= data.shorts.size()) return;

    data.shorts[index] = value;
}

void GlobalStoreXExtension::AddInteger(int index, int value)
{
    index -= data.integerBase;
    if (index < 0 || index >= data.integers.size()) return;

    data.integers[index] += value;
}

void GlobalStoreXExtension::AddShort(int index, short value)
{
    index -= data.shortBase;
    if (index < 0 || index >= data.shorts.size()) return;

    data.shorts[index] += value;
}

void GlobalStoreXExtension::AddString(int index, const std::string& value)
{
    index -= data.stringBase;
    if (index < 0 || index >= data.strings.size()) return;

    data.strings[index] += value;
}

void GlobalStoreXExtension::SubtractInteger(int index, int value)
{
    index -= data.integerBase;
    if (index < 0 || index >= data.integers.size()) return;

    data.integers[index] -= value;
}

void GlobalStoreXExtension::SubtractShort(int index, short value)
{
    index -= data.shortBase;
    if (index < 0 || index >= data.shorts.size()) return;

    data.shorts[index] -= value;
}

void GlobalStoreXExtension::SetRangeInteger(int startIndex, int endIndex, int value)
{
    if (data.integerCount == 0) return;

    startIndex -= data.integerBase;
    endIndex -= data.integerBase;

    if (startIndex < 0) startIndex = 0;
    if (endIndex >= (int)data.integerCount) endIndex = (int)data.integerCount - 1;
    if (startIndex >= (int)data.integerCount || endIndex < startIndex) return;

    for (int i = startIndex; i <= endIndex; i++)
    {
        data.integers[i] = value;
    }
}

void GlobalStoreXExtension::SetRangeString(int startIndex, int endIndex, const std::string& value)
{
    if (data.stringCount == 0) return;

    startIndex -= data.stringBase;
    endIndex -= data.stringBase;

    if (startIndex < 0) startIndex = 0;
    if (endIndex >= (int)data.stringCount) endIndex = (int)data.stringCount - 1;
    if (startIndex >= (int)data.stringCount || endIndex < startIndex) return;

    for (int i = startIndex; i <= endIndex; i++)
    {
        data.strings[i] = value;
    }
}

void GlobalStoreXExtension::SetRangeBool(int startIndex, int endIndex, int value)
{
    if (data.boolCount == 0) return;

    startIndex -= data.boolBase;
    endIndex -= data.boolBase;

    if (startIndex < 0) startIndex = 0;
    if (endIndex >= (int)data.boolCount) endIndex = (int)data.boolCount - 1;
    if (startIndex >= (int)data.boolCount || endIndex < startIndex) return;

    for (int i = startIndex; i <= endIndex; i++)
    {
        data.bools[i] = value != 0;
    }
}

void GlobalStoreXExtension::SetRangeShort(int startIndex, int endIndex, short value)
{
    if (data.shortCount == 0) return;

    startIndex -= data.shortBase;
    endIndex -= data.shortBase;

    if (startIndex < 0) startIndex = 0;
    if (endIndex >= (int)data.shortCount) endIndex = (int)data.shortCount - 1;
    if (startIndex >= (int)data.shortCount || endIndex < startIndex) return;

    for (int i = startIndex; i <= endIndex; i++)
    {
        data.shorts[i] = value;
    }
}

void GlobalStoreXExtension::ClearIntegers()
{
    data.integers.clear();
    data.integers.resize(data.integerCount);
}

void GlobalStoreXExtension::ClearStrings()
{
    data.strings.clear();
    data.strings.resize(data.stringCount);
}

void GlobalStoreXExtension::ClearBools()
{
    data.bools.clear();
    data.bools.resize(data.boolCount);
}

void GlobalStoreXExtension::ClearShorts()
{
    data.shorts.clear();
    data.shorts.resize(data.shortCount);
}

void GlobalStoreXExtension::ResizeIntegers(int size)
{
    data.integers.clear();
    data.integers.resize(size);
    data.integerCount = size;
}

void GlobalStoreXExtension::ResizeStrings(int size)
{
    data.strings.clear();
    data.strings.resize(size);
    data.stringCount = size;
}

void GlobalStoreXExtension::ResizeBools(int size)
{
    data.bools.clear();
    data.bools.resize(size);
    data.boolCount = size;
}

void GlobalStoreXExtension::ResizeShorts(int size)
{
    data.shorts.clear();
    data.shorts.resize(size);
    data.shortCount = size;
}

void GlobalStoreXExtension::ExpandIntegers(int size)
{
    data.integers.resize(size);
    data.integerCount = size;
}

void GlobalStoreXExtension::ExpandStrings(int size)
{
    data.strings.resize(size);
    data.stringCount = size;
}

void GlobalStoreXExtension::ExpandBools(int size)
{
    data.bools.resize(size);
    data.boolCount = size;
}

void GlobalStoreXExtension::ExpandShorts(int size)
{
    data.shorts.resize(size);
    data.shortCount = size;
}

int GlobalStoreXExtension::GetInteger(int index)
{
    index -= data.integerBase;
    if (index < 0 || index >= data.integers.size()) return 0;

    return data.integers[index];
}

std::string GlobalStoreXExtension::GetString(int index)
{
    index -= data.stringBase;
    if (index < 0 || index >= data.strings.size()) return "";

    return data.strings[index];
}

bool GlobalStoreXExtension::GetBool(int index)
{
    index -= data.boolBase;
    if (index < 0 || index >= data.bools.size()) return false;

    return data.bools[index];
}

short GlobalStoreXExtension::GetShort(int index)
{
    index -= data.shortBase;
    if (index < 0 || index >= data.shorts.size()) return 0;

    return data.shorts[index];
}

void GlobalStoreXExtension::SaveAllINI(const std::string& fileName, const std::string& group)
{
    mINI::INIFile file(GetNormalizedPath(fileName));
    mINI::INIStructure ini;

    file.read(ini);

    ini[group + "Ints"].clear();
    ini[group + "Ints"]["NoOfValues"] = std::to_string(data.integerCount);
    ini[group + "Ints"]["Base"] = std::to_string(data.integerBase);
    for (int i = 0; i < data.integerCount; i++)
    {
        ini[group + "Ints"][std::to_string(i)] = std::to_string(data.integers[i]);
    }

    ini[group + "Strings"].clear();
    ini[group + "Strings"]["NoOfStrings"] = std::to_string(data.stringCount);
    ini[group + "Strings"]["Base"] = std::to_string(data.stringBase);
    for (int i = 0; i < data.stringCount; i++)
    {
        ini[group + "Strings"][std::to_string(i)] = data.strings[i];
    }

    ini[group + "Bools"].clear();
    ini[group + "Bools"]["NoOfBools"] = std::to_string(data.boolCount);
    ini[group + "Bools"]["Base"] = std::to_string(data.boolBase);
    for (int i = 0; i < data.boolCount; i++)
    {
        ini[group + "Bools"][std::to_string(i)] = std::to_string(data.bools[i]);
    }
    
    ini[group + "Shorts"].clear();
    ini[group + "Shorts"]["NoOfShorts"] = std::to_string(data.shortCount);
    ini[group + "Shorts"]["Base"] = std::to_string(data.shortBase);
    for (int i = 0; i < data.shortCount; i++)
    {
        ini[group + "Shorts"][std::to_string(i)] = std::to_string(data.shorts[i]);
    }

    file.write(ini);
}

void GlobalStoreXExtension::LoadAllINI(const std::string& fileName, const std::string& group)
{
    mINI::INIFile file(GetNormalizedPath(fileName));
    mINI::INIStructure ini;

    if (!file.read(ini)) return;

    data.integerBase = ini[group + "Ints"]["Base"] == "0" ? 0 : 1;
    data.stringBase = ini[group + "Strings"]["Base"] == "0" ? 0 : 1;
    data.boolBase = ini[group + "Bools"]["Base"] == "0" ? 0 : 1;
    data.shortBase = ini[group + "Shorts"]["Base"] == "0" ? 0 : 1;

    data.integerCount = std::stoi(ini[group + "Ints"]["NoOfValues"]);
    data.integers.clear();
    data.integers.resize(data.integerCount);
    for (int i = 0; i < data.integerCount; i++)
    {
        data.integers[i] = std::stoi(ini[group + "Ints"][std::to_string(i)]);
    }

    data.stringCount = std::stoi(ini[group + "Strings"]["NoOfStrings"]);
    data.strings.clear();
    data.strings.resize(data.stringCount);
    for (int i = 0; i < data.stringCount; i++)
    {
        data.strings[i] = ini[group + "Strings"][std::to_string(i)];
    }

    data.boolCount = std::stoi(ini[group + "Bools"]["NoOfBools"]);
    data.bools.clear();
    data.bools.resize(data.boolCount);
    for (int i = 0; i < data.boolCount; i++)
    {
        data.bools[i] = std::stoi(ini[group + "Bools"][std::to_string(i)]) != 0;
    }

    data.shortCount = std::stoi(ini[group + "Shorts"]["NoOfShorts"]);
    data.shorts.clear();
    data.shorts.resize(data.shortCount);
    for (int i = 0; i < data.shortCount; i++)
    {
        data.shorts[i] = std::stoi(ini[group + "Shorts"][std::to_string(i)]);
    }
}

void GlobalStoreXExtension::SaveIntegerINI(const std::string& fileName, const std::string& group)
{
    mINI::INIFile file(GetNormalizedPath(fileName));
    mINI::INIStructure ini;

    file.read(ini);

    ini[group + "Ints"].clear();
    ini[group + "Ints"]["NoOfValues"] = std::to_string(data.integerCount);
    ini[group + "Ints"]["Base"] = std::to_string(data.integerBase);
    for (int i = 0; i < data.integerCount; i++)
    {
        ini[group + "Ints"][std::to_string(i)] = std::to_string(data.integers[i]);
    }

    file.write(ini);
}

void GlobalStoreXExtension::LoadIntegerINI(const std::string& fileName, const std::string& group)
{
    mINI::INIFile file(GetNormalizedPath(fileName));
    mINI::INIStructure ini;

    if (!file.read(ini)) return;
    
    data.integerBase = ini[group + "Ints"]["Base"] == "0" ? 0 : 1;

    data.integerCount = std::stoi(ini[group + "Ints"]["NoOfValues"]);
    data.integers.clear();
    data.integers.resize(data.integerCount);
    for (int i = 0; i < data.integerCount; i++)
    {
        data.integers[i] = std::stoi(ini[group + "Ints"][std::to_string(i)]);
    }
}

void GlobalStoreXExtension::SaveStringINI(const std::string& fileName, const std::string& group)
{
    mINI::INIFile file(GetNormalizedPath(fileName));
    mINI::INIStructure ini;

    file.read(ini);

    ini[group + "Strings"].clear();
    ini[group + "Strings"]["NoOfStrings"] = std::to_string(data.stringCount);
    ini[group + "Strings"]["Base"] = std::to_string(data.stringBase);
    for (int i = 0; i < data.stringCount; i++)
    {
        ini[group + "Strings"][std::to_string(i)] = data.strings[i];
    }

    file.write(ini);
}

void GlobalStoreXExtension::LoadStringINI(const std::string& fileName, const std::string& group)
{
    mINI::INIFile file(GetNormalizedPath(fileName));
    mINI::INIStructure ini;

    if (!file.read(ini)) return;

    data.stringBase = ini[group + "Strings"]["Base"] == "0" ? 0 : 1;

    data.stringCount = std::stoi(ini[group + "Strings"]["NoOfStrings"]);
    data.strings.clear();
    data.strings.resize(data.stringCount);

    for (int i = 0; i < data.stringCount; i++)
    {
        data.strings[i] = ini[group + "Strings"][std::to_string(i)];
    }
}

void GlobalStoreXExtension::SaveBoolINI(const std::string& fileName, const std::string& group)
{
    mINI::INIFile file(GetNormalizedPath(fileName));
    mINI::INIStructure ini;

    file.read(ini);

    ini[group + "Bools"].clear();
    ini[group + "Bools"]["NoOfBools"] = std::to_string(data.boolCount);
    ini[group + "Bools"]["Base"] = std::to_string(data.boolBase);
    for (int i = 0; i < data.boolCount; i++)
    {
        ini[group + "Bools"][std::to_string(i)] = std::to_string(data.bools[i]);
    }

    file.write(ini);
}

void GlobalStoreXExtension::LoadBoolINI(const std::string& fileName, const std::string& group)
{
    mINI::INIFile file(GetNormalizedPath(fileName));
    mINI::INIStructure ini;

    if (!file.read(ini)) return;

    data.boolBase = ini[group + "Bools"]["Base"] == "0" ? 0 : 1;

    data.boolCount = std::stoi(ini[group + "Bools"]["NoOfBools"]);
    data.bools.clear();
    data.bools.resize(data.boolCount);
    for (int i = 0; i < data.boolCount; i++)
    {
        data.bools[i] = std::stoi(ini[group + "Bools"][std::to_string(i)]) != 0;
    }
}

void GlobalStoreXExtension::SaveShortINI(const std::string& fileName)
{
    mINI::INIFile file(GetNormalizedPath(fileName));
    mINI::INIStructure ini;

    file.read(ini);

    ini["Shorts"].clear();
    ini["Shorts"]["NoOfShorts"] = std::to_string(data.shortCount);
    ini["Shorts"]["Base"] = std::to_string(data.shortBase);
    for (int i = 0; i < data.shortCount; i++)
    {
        ini["Shorts"][std::to_string(i)] = std::to_string(data.shorts[i]);
    }

    file.write(ini);
}

void GlobalStoreXExtension::LoadShortINI(const std::string& fileName)
{
    mINI::INIFile file(GetNormalizedPath(fileName));
    mINI::INIStructure ini;
    
    if (!file.read(ini)) return;

    data.shortBase = ini["Shorts"]["Base"] == "0" ? 0 : 1;

    data.shortCount = std::stoi(ini["Shorts"]["NoOfShorts"]);
    data.shorts.clear();
    data.shorts.resize(data.shortCount);
    for (int i = 0; i < data.shortCount; i++)
    {
        data.shorts[i] = std::stoi(ini["Shorts"][std::to_string(i)]);
    }
}

void GlobalStoreXExtension::SaveAllBinary(const std::string& fileName)
{
    BinaryWriter writer(GetNormalizedPath(fileName));

    writer.WriteUint16(3); // type of file type. 0 - ints; 1 - strings; 2 - bools; 3 - all data; shorts - 4

    writer.WriteUint32(data.integerCount);
    writer.WriteUint8(data.integerBase);
    for (int i = 0; i < data.integerCount; i++)
    {
        writer.WriteInt32(data.integers[i]);
    }

    writer.WriteUint32(data.boolCount);
    writer.WriteUint8(data.boolBase);
    for (int i = 0; i < data.boolCount; i++)
    {
        writer.WriteUint8(data.bools[i] ? static_cast<uint8_t>(1) : static_cast<uint8_t>(0));
    }

    writer.WriteUint32(data.shortCount);
    writer.WriteUint8(data.shortBase);
    for (int i = 0; i < data.shortCount; i++)
    {
        writer.WriteInt16(data.shorts[i]);
    }

    writer.WriteUint32(data.stringCount);
    writer.WriteUint8(data.stringBase);
    for (int i = 0; i < data.stringCount; i++)
    {
        writer.WriteNullTerminatedString(data.strings[i]);
    }

    writer.Flush();
}

void GlobalStoreXExtension::LoadAllBinary(const std::string& fileName)
{
    if (!std::filesystem::exists(GetNormalizedPath(fileName))) return;

    BinaryReader reader(GetNormalizedPath(fileName));

    if (reader.ReadUint16() != 3) return;

    data.integerCount = reader.ReadUint32();
    data.integerBase = reader.ReadUint8();
    data.integers.resize(data.integerCount);
    for (int i = 0; i < data.integerCount; i++)
    {
        data.integers[i] = reader.ReadInt32();
    }

    data.boolCount = reader.ReadUint32();
    data.boolBase = reader.ReadUint8();
    data.bools.resize(data.boolCount);
    for (int i = 0; i < data.boolCount; i++)
    {
        data.bools[i] = reader.ReadUint8() != 0;
    }

    data.shortCount = reader.ReadUint32();
    data.shortBase = reader.ReadUint8();
    data.shorts.resize(data.shortCount);
    for (int i = 0; i < data.shortCount; i++)
    {
        data.shorts[i] = reader.ReadInt16();
    }

    data.stringCount = reader.ReadUint32();
    data.stringBase = reader.ReadUint8();
    data.strings.resize(data.stringCount);
    for (int i = 0; i < data.stringCount; i++)
    {
        data.strings[i] = reader.ReadNullTerminatedString();
    }
}

void GlobalStoreXExtension::SaveIntegerBinary(const std::string& fileName)
{
    BinaryWriter writer(GetNormalizedPath(fileName));

    writer.WriteUint16(0);
    writer.WriteUint32(data.integerCount);
    writer.WriteUint8(data.integerBase);
    for (int i = 0; i < data.integerCount; i++)
    {
        writer.WriteInt32(data.integers[i]);
    }

    writer.Flush();
}

void GlobalStoreXExtension::LoadIntegerBinary(const std::string& fileName)
{
    if (!std::filesystem::exists(GetNormalizedPath(fileName))) return;

    BinaryReader reader(GetNormalizedPath(fileName));

    if (reader.ReadUint16() != 0) return;

    data.integerCount = reader.ReadUint32();
    data.integerBase = reader.ReadUint8();
    data.integers.resize(data.integerCount);
    for (int i = 0; i < data.integerCount; i++)
    {
        data.integers[i] = reader.ReadInt32();
    }
}

void GlobalStoreXExtension::SaveStringBinary(const std::string& fileName)
{
    BinaryWriter writer(GetNormalizedPath(fileName));

    writer.WriteUint16(1);
    writer.WriteUint32(data.stringCount);
    writer.WriteUint8(data.stringBase);
    for (int i = 0; i < data.stringCount; i++)
    {
        writer.WriteNullTerminatedString(data.strings[i]);
    }

    writer.Flush();
}

void GlobalStoreXExtension::LoadStringBinary(const std::string& fileName)
{
    if (!std::filesystem::exists(GetNormalizedPath(fileName))) return;

    BinaryReader reader(GetNormalizedPath(fileName));

    if (reader.ReadUint16() != 1) return;

    data.stringCount = reader.ReadUint32();
    data.stringBase = reader.ReadUint8();
    data.strings.resize(data.stringCount);
    for (int i = 0; i < data.stringCount; i++)
    {
        data.strings[i] = reader.ReadNullTerminatedString();
    }
}

void GlobalStoreXExtension::SaveBoolBinary(const std::string& fileName)
{
    BinaryWriter writer(GetNormalizedPath(fileName));

    writer.WriteUint16(2);
    writer.WriteUint32(data.boolCount);
    writer.WriteUint8(data.boolBase);
    for (int i = 0; i < data.boolCount; i++)
    {
        writer.WriteUint8(data.bools[i] ? static_cast<uint8_t>(1) : static_cast<uint8_t>(0));
    }

    writer.Flush();
}

void GlobalStoreXExtension::LoadBoolBinary(const std::string& fileName)
{
    if (!std::filesystem::exists(GetNormalizedPath(fileName))) return;

    BinaryReader reader(GetNormalizedPath(fileName));

    if (reader.ReadUint16() != 2) return;

    data.boolCount = reader.ReadUint32();
    data.boolBase = reader.ReadUint8();
    data.bools.resize(data.boolCount);
    for (int i = 0; i < data.boolCount; i++)
    {
        data.bools[i] = reader.ReadUint8() != 0;
    }
}

void GlobalStoreXExtension::SaveShortBinary(const std::string& fileName)
{
    BinaryWriter writer(GetNormalizedPath(fileName));

    writer.WriteUint16(4);
    writer.WriteUint32(data.shortCount);
    writer.WriteUint8(data.shortBase);
    for (int i = 0; i < data.shortCount; i++)
    {
        writer.WriteInt16(data.shorts[i]);
    }

    writer.Flush();
}

void GlobalStoreXExtension::LoadShortBinary(const std::string& fileName)
{
    if (!std::filesystem::exists(GetNormalizedPath(fileName))) return;

    BinaryReader reader(GetNormalizedPath(fileName));

    if (reader.ReadUint16() != 4) return;

    data.shortCount = reader.ReadUint32();
    data.shortBase = reader.ReadUint8();
    data.shorts.resize(data.shortCount);
    for (int i = 0; i < data.shortCount; i++)
    {
        data.shorts[i] = reader.ReadInt16();
    }
}

std::string GlobalStoreXExtension::GetNormalizedPath(const std::string& path)
{
#if defined(PLATFORM_WEB)
    return "/disk/" + path;
#else
    return path;
#endif
}