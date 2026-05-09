#pragma once

#include "EffectInstance.h"

class EffectBank {
public:
    static EffectBank& Instance() {
        static EffectBank instance;
        return instance;
    }

    {{ EFFECT_DEFINITIONS }}
}; 