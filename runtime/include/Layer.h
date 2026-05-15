#pragma once

#include <string>
#include <vector>

class EffectInstance;
class ObjectInstance;

class Layer {
public:
	Layer(std::string name, float XCoefficient, float YCoefficient)
		: Name(name)
		, XCoefficient(XCoefficient)
		, YCoefficient(YCoefficient)
	{
	}

	std::string Name;

	float XCoefficient;
	float YCoefficient;

	int RGBCoefficient = 0xFFFFFFFF;
    int Effect = 0;
	bool usePreviousLayerEffect = false;
    EffectInstance* effectInstance = nullptr;

	std::vector<ObjectInstance*> instances;

	unsigned char GetEffectParameter() const {
        return EffectParameter;
    }
    
    void SetEffectParameter(int effectParameter) {
        EffectParameter = static_cast<unsigned char>(std::clamp(effectParameter, 0, 255));
    }

private:
    unsigned char EffectParameter = 0;
};
