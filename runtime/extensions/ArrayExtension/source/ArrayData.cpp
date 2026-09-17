#include "ArrayData.h"

ArrayData::ArrayData(int newDimensionX, int newDimensionY, int newDimensionZ, int flags)
{
    dimensionX = std::max(1, newDimensionX);
    dimensionY = std::max(1, newDimensionY);
    dimensionZ = std::max(1, newDimensionZ);
    flags = flags;

    Clear();
}

void ArrayData::Clear()
{
    if (IsNumberArray()) {
        intData.assign(dimensionX * dimensionY * dimensionZ, 0); 
    } else if (IsStringArray()) {
        stringData.assign(dimensionX * dimensionY * dimensionZ, "");
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
        std::vector<std::string> newData(requestedDimensionX * requestedDimensionY * requestedDimensionZ, "");

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
        stringData[(static_cast<size_t>(xInt) * dimensionY * dimensionZ) + (yInt * dimensionZ) + zInt] = value.GetStringValue();
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
        return CValue(stringData[(static_cast<size_t>(xInt) * dimensionY * dimensionZ) + (yInt * dimensionZ) + zInt]);
    }

    return CValue(0);
}