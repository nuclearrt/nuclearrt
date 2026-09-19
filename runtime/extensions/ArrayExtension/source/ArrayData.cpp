#include "ArrayData.h"

#include "BinaryTools/BinaryReader.h"
#include "BinaryTools/BinaryWriter.h"

#include <cwchar>

#include <iostream>

#include <string>

namespace {
std::wstring ToWide(const std::string& narrow)
{
    if (narrow.empty())
        return {};

    const size_t sizeNeeded = std::mbstowcs(nullptr, narrow.c_str(), 0);
    if (sizeNeeded == static_cast<size_t>(-1))
        return std::wstring(narrow.begin(), narrow.end());

    std::wstring wide(sizeNeeded, L'\0');
    std::mbstowcs(wide.data(), narrow.c_str(), sizeNeeded);
    return wide;
}

std::string ToNarrow(const std::wstring& wide)
{
    if (wide.empty())
        return {};

    const size_t sizeNeeded = std::wcstombs(nullptr, wide.c_str(), 0);

    if (sizeNeeded == static_cast<size_t>(-1))
        return std::string(wide.begin(), wide.end());

    std::string narrow(sizeNeeded, '\0');

    std::wcstombs(narrow.data(), wide.c_str(), sizeNeeded);
    return narrow;
}
}

ArrayData::ArrayData(int newDimensionX, int newDimensionY, int newDimensionZ, int flags)
{
    dimensionX = std::max(1, newDimensionX);
    dimensionY = std::max(1, newDimensionY);
    dimensionZ = std::max(1, newDimensionZ);
    this->flags = flags;

    Clear();
}

void ArrayData::Clear()
{
    if (IsNumberArray()) {
        intData.assign(dimensionX * dimensionY * dimensionZ, 0); 
    } else if (IsStringArray()) {
        stringData.assign(dimensionX * dimensionY * dimensionZ, L"");
    }
}

void ArrayData::Expand(int newDimensionX, int newDimensionY, int newDimensionZ)
{
    int requestedDimensionX = std::max(1, newDimensionX);
    int requestedDimensionY = std::max(1, newDimensionY);
    int requestedDimensionZ = std::max(1, newDimensionZ);

    if (requestedDimensionX < dimensionX && requestedDimensionY < dimensionY && requestedDimensionZ < dimensionZ) return;

    if (IsNumberArray()) {
        std::vector<int> newData(requestedDimensionX * requestedDimensionY * requestedDimensionZ, 0);

        for (int x = 0; x < dimensionX; ++x) {
            for (int y = 0; y < dimensionY; ++y) {
                for (int z = 0; z < dimensionZ; ++z) {
                    newData[(static_cast<size_t>(x) * requestedDimensionY * requestedDimensionZ) + (y * requestedDimensionZ) + z] = intData[(static_cast<size_t>(x) * dimensionY * dimensionZ) + (y * dimensionZ) + z];
                }
            }
        }
        intData.swap(newData);
    } 
    else if (IsStringArray()) {
        std::vector<std::wstring> newData(requestedDimensionX * requestedDimensionY * requestedDimensionZ, L"");

        for (int x = 0; x < dimensionX; ++x) {
            for (int y = 0; y < dimensionY; ++y) {
                for (int z = 0; z < dimensionZ; ++z) {
                    newData[(static_cast<size_t>(x) * requestedDimensionY * requestedDimensionZ) + (y * requestedDimensionZ) + z] = std::move(stringData[(static_cast<size_t>(x) * dimensionY * dimensionZ) + (y * dimensionZ) + z]);
                }
            }
        }
        stringData.swap(newData);
    }

    dimensionX = requestedDimensionX;
    dimensionY = requestedDimensionY;
    dimensionZ = requestedDimensionZ;
}

void ArrayData::WriteXYZ(const CValue& x, const CValue& y, const CValue& z, const CValue& value)
{
    int xInt = x.GetIntValue();
    int yInt = y.GetIntValue();
    int zInt = z.GetIntValue();

    xInt -= GetBaseOffset();
    yInt -= GetBaseOffset();
    zInt -= GetBaseOffset();

    if (xInt < 0 || yInt < 0 || zInt < 0)
        return;

    if (xInt >= dimensionX || yInt >= dimensionY || zInt >= dimensionZ)
        Expand(std::max(dimensionX, xInt + 1), std::max(dimensionY, yInt + 1), std::max(dimensionZ, zInt + 1));

    currentIndexX = xInt;
    currentIndexY = yInt;
    currentIndexZ = zInt;

    if (IsNumberArray()) {
        intData[(static_cast<size_t>(xInt) * dimensionY * dimensionZ) + (yInt * dimensionZ) + zInt] = value.GetIntValue();
    } else if (IsStringArray()) {
        stringData[(static_cast<size_t>(xInt) * dimensionY * dimensionZ) + (yInt * dimensionZ) + zInt] = ToWide(value.GetStringValue());
    }
}

CValue ArrayData::GetValueAtXYZ(const CValue& x, const CValue& y, const CValue& z)
{
    int xInt = x.GetIntValue();
    int yInt = y.GetIntValue();
    int zInt = z.GetIntValue();

    xInt -= GetBaseOffset();
    yInt -= GetBaseOffset();
    zInt -= GetBaseOffset();

    if (xInt < 0 || yInt < 0 || zInt < 0)
        return CValue(0);

    if (xInt >= dimensionX || yInt >= dimensionY || zInt >= dimensionZ)
        return CValue(0);

    if (IsNumberArray()) {
        return CValue(intData[(static_cast<size_t>(xInt) * dimensionY * dimensionZ) + (yInt * dimensionZ) + zInt]);
    } else if (IsStringArray()) {
        return CValue(ToNarrow(stringData[(static_cast<size_t>(xInt) * dimensionY * dimensionZ) + (yInt * dimensionZ) + zInt]));
    }

    return CValue(0);
}

void ArrayData::Save(const std::string& fileName)
{
    BinaryWriter writer(GetNormalizedPath(fileName));

    writer.WriteNullTerminatedString("MFU ARRAY");
    writer.WriteUint16(2);
    writer.WriteUint16(0);
    writer.WriteUint32(dimensionX);
    writer.WriteUint32(dimensionY);
    writer.WriteUint32(dimensionZ);
    writer.WriteUint32(flags);

    if (IsNumberArray()) {
        for (int z = 0; z < dimensionZ; ++z) {
            for (int y = 0; y < dimensionY; ++y) {
                for (int x = 0; x < dimensionX; ++x) {
                    writer.WriteUint32(intData[(static_cast<size_t>(x) * dimensionY * dimensionZ) + (y * dimensionZ) + z]);
                }
            }
        }
    }
    else if (IsStringArray()) {
        for (int z = 0; z < dimensionZ; ++z) {
            for (int y = 0; y < dimensionY; ++y) {
                for (int x = 0; x < dimensionX; ++x) {
                    const std::wstring& value = stringData[(static_cast<size_t>(x) * dimensionY * dimensionZ) + (y * dimensionZ) + z];
                    writer.WriteUint32(static_cast<uint32_t>(value.length()));
                    writer.WriteFixedLengthStringWide(value);
                }
            }
        }
    }

    writer.Flush();
}

void ArrayData::Load(const std::string& fileName)
{
    BinaryReader reader(GetNormalizedPath(fileName));

    std::string magic = reader.ReadNullTerminatedString();

    bool unicode = false;

    if (magic == "MFU ARRAY")
    {
        unicode = true;
    }
    else if (magic == "CNC ARRAY")
    {
        unicode = false;
    }
    else
    {
        return;
    }

    unsigned short version = reader.ReadUint16();
    unsigned short revision = reader.ReadUint16();

    if (version != 2 || revision != 0) return;

    int dimX = reader.ReadUint32();
    int dimY = reader.ReadUint32();
    int dimZ = reader.ReadUint32();
    int loadedFlags = reader.ReadUint32();

    if (dimX < 0 || dimY < 0 || dimZ < 0) return;

    flags = loadedFlags;

    if (IsNumberArray()) {
        intData.assign(dimX * dimY * dimZ, 0);
        for (int z = 0; z < dimZ; ++z) {
            for (int y = 0; y < dimY; ++y) {
                for (int x = 0; x < dimX; ++x) {
                    intData[(static_cast<size_t>(x) * dimY * dimZ) + (y * dimZ) + z] = reader.ReadUint32();
                }
            }
        }
    }
    else if (IsStringArray()) {
        stringData.assign(dimX * dimY * dimZ, L"");
        for (int z = 0; z < dimZ; ++z) {
            for (int y = 0; y < dimY; ++y) {
                for (int x = 0; x < dimX; ++x) {
                    int length = reader.ReadUint32();

                    if (unicode)
                        stringData[(static_cast<size_t>(x) * dimY * dimZ) + (y * dimZ) + z] = reader.ReadFixedLengthStringWide(length);
                    else
                        stringData[(static_cast<size_t>(x) * dimY * dimZ) + (y * dimZ) + z] = ToWide(reader.ReadFixedLengthString(length));
                }
            }
        }
    }

    dimensionX = dimX;
    dimensionY = dimY;
    dimensionZ = dimZ;

    currentIndexX = 0;
    currentIndexY = 0;
    currentIndexZ = 0;
}

std::string ArrayData::GetNormalizedPath(const std::string& path)
{
#if defined(PLATFORM_WEB)
    return "/disk/" + path;
#else
    return path;
#endif
}