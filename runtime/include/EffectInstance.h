#pragma once

#include <variant>
#include <string>
#include <vector>

struct EffectParameter {
    std::string Name;
    int Type;
    std::variant<int, float> Value;

    EffectParameter(std::string name, int type, std::variant<int, float> value)
        : Name(name), Type(type), Value(value) {}
};

class EffectInstance {
public:
    EffectInstance(std::string filename, std::vector<EffectParameter> parameters)
        : filename(filename), Parameters(parameters) {}

    std::string filename;
    std::vector<EffectParameter> Parameters;

    void SetParameter(std::string name, std::variant<int, float> value)
    {
        for (auto& parameter : Parameters) {
            if (parameter.Name == name) {
                if (parameter.Type == 1)
                {
                    if (std::holds_alternative<float>(value))
                        parameter.Value = std::get<float>(value);
                    else
                        parameter.Value = static_cast<float>(std::get<int>(value));
                }
                else
                {
                    if (std::holds_alternative<int>(value))
                        parameter.Value = std::get<int>(value);
                    else
                        parameter.Value = static_cast<int>(std::get<float>(value));
                }

                return;
            }
        }
    }
};