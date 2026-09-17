#pragma once

#include <string>
#include <vector>
#include "CValue.h"

class ArrayData {
public:
    ArrayData(int dimensionX, int dimensionY, int dimensionZ, int flags);
    ~ArrayData();

    bool IsNumberArray() const { return (flags & 0x0001) != 0; }
    bool IsStringArray() const { return (flags & 0x0002) != 0; }
    bool IsIndexBase1() const { return (flags & 0x0004) != 0; }

    int GetBaseOffset() const { return IsIndexBase1() ? 0 : 1; }

    void SetIndexA(const CValue& index) { currentIndexX = index.GetIntValue() - GetBaseOffset(); }
    void SetIndexB(const CValue& index) { currentIndexY = index.GetIntValue() - GetBaseOffset(); }
    void SetIndexC(const CValue& index) { currentIndexZ = index.GetIntValue() - GetBaseOffset(); }

    void AddIndexA() { currentIndexX++; }
    void AddIndexB() { currentIndexY++; }
    void AddIndexC() { currentIndexZ++; }

    void Clear();
    void Expand(int newDimensionX, int newDimensionY, int newDimensionZ);

    void WriteValueAtIndex(const CValue& value) { WriteXYZ(CValue(currentIndexX), CValue(currentIndexY), CValue(currentIndexZ), value); }

    void WriteX(const CValue& x, const CValue& value) { WriteXYZ(x, CValue(currentIndexY), CValue(currentIndexZ), value); }
    void WriteXY(const CValue& x, const CValue& y, const CValue& value) { WriteXYZ(x, y, CValue(currentIndexZ), value); }
    void WriteXYZ(const CValue& x, const CValue& y, const CValue& z, const CValue& value);

    bool IsXAtEnd() { return currentIndexX >= dimensionX - 1; }
    bool IsYAtEnd() { return currentIndexY >= dimensionY - 1; }
    bool IsZAtEnd() { return currentIndexZ >= dimensionZ - 1; }

    int GetXIndex() { return currentIndexX + GetBaseOffset(); }
    int GetYIndex() { return currentIndexY + GetBaseOffset(); }
    int GetZIndex() { return currentIndexZ + GetBaseOffset(); }

    CValue GetValueAtIndex() { return GetValueAtXYZ(currentIndexX, currentIndexY, currentIndexZ); }

    CValue GetValueAtX(const CValue& x) { return GetValueAtXYZ(x, currentIndexY, currentIndexZ); }
    CValue GetValueAtXY(const CValue& x, const CValue& y) { return GetValueAtXYZ(x, y, currentIndexZ); }
    CValue GetValueAtXYZ(const CValue& x, const CValue& y, const CValue& z);

    CValue GetXDimension() { return CValue(dimensionX); }
    CValue GetYDimension() { return CValue(dimensionY); }
    CValue GetZDimension() { return CValue(dimensionZ); }

private:
    int dimensionX;
    int dimensionY;
    int dimensionZ;
    int flags;

    int currentIndexX = 0;
    int currentIndexY = 0;
    int currentIndexZ = 0;

    std::vector<int> intData;
    std::vector<std::string> stringData;
};