#pragma once

#include <vector>
#include <memory>

#include "CounterBase.h"

class Lives : public CounterBase {
public:
	Lives(unsigned int objectInfoHandle, int type, std::string name)
		: CounterBase(objectInfoHandle, type, name) {}

	int GetValue() const override { return Application::Instance().GetAppData()->GetPlayerLives()[Player]; }
};