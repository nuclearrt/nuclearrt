#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "ObjectGlobalData.h"
#include "EffectInstance.h"

class ObjectInstance {
public:
    ObjectInstance(unsigned int objectInfoHandle, int type, std::string name)
        : ObjectInfoHandle(objectInfoHandle), Type(type), Name(name) {}
    virtual ~ObjectInstance() = default;
    
    std::string Name = "";
    std::vector<short> Qualifiers = {};
    
    unsigned int Handle = 0;
    unsigned int Type = 0;
    unsigned int ObjectInfoHandle = 0;
	unsigned int Layer = 0;
private:
    unsigned int Angle = 0;
public:

	int X = 0;
    int Y = 0;
    int RGBCoefficient = 0xFFFFFF;
    int Effect = 0;
    EffectInstance* effectInstance = nullptr;

    short InstanceValue = 0;
    
    bool global = false;
	bool isSelected = false;
private:
    unsigned char EffectParameter = 0;
public:


    unsigned char GetEffectParameter() const {
        return EffectParameter;
    }
    
    void SetEffectParameter(int effectParameter) {
        EffectParameter = static_cast<unsigned char>(std::clamp(effectParameter, 0, 255));
    }
    
    unsigned int GetAngle() const {
        return Angle;
    }

    void SetAngle(int angle) {
        angle %= 360;
        if (angle < 0) angle += 360;
        Angle = static_cast<unsigned int>(angle);
    }

    virtual ObjectGlobalData* CreateGlobalData() { return nullptr; };
    virtual void ApplyGlobalData(ObjectGlobalData* globalData) { };

    virtual std::vector<unsigned int> GetImagesUsed() { return std::vector<unsigned int>(); };
    virtual std::vector<unsigned int> GetFontsUsed() { return std::vector<unsigned int>(); };

};