#pragma once

#include <any>
#include <string>
#include <vector>

struct EffectParameter {
    std::string Name;
    int Type;
    std::any Value;

    EffectParameter(std::string name, int type, std::any value)
        : Name(name), Type(type), Value(value) {}
};

class EffectInstance {
public:
    EffectInstance(std::string filename, std::vector<EffectParameter> parameters)
        : filename(filename), Parameters(parameters) {}

    std::string filename;
    std::vector<EffectParameter> Parameters;

    void SetParameter(std::string name, std::any value) {
        for (auto& parameter : Parameters) {
            if (parameter.Name == name) {
                parameter.Value = value;
                return;
            }
        }
    }
};