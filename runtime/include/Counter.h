#pragma once

#include <vector>
#include <memory>

#include "CounterBase.h"
#include "ObjectGlobalDataCounter.h"

class Counter : public CounterBase {
public:
	Counter(unsigned int objectInfoHandle, int type, std::string name)
		: CounterBase(objectInfoHandle, type, name) {}
	
	int DefaultValue = 0;
	int MinValue = 0;
	int MaxValue = 0;

	int GetValue() const override { return currentValue; }
	void SetValue(int value)
	{
		if (value < MinValue)
		{
			currentValue = MinValue;
		}
		else if (value > MaxValue)
		{
			currentValue = MaxValue;
		}
		else
		{
			currentValue = value;
		}
	}

	void AddValue(int value)
	{
		SetValue(currentValue + value);
	}

	void SubtractValue(int value)
	{
		SetValue(currentValue - value);
	}

	ObjectGlobalDataCounter* CreateGlobalData() override {
		ObjectGlobalDataCounter* globalData = new ObjectGlobalDataCounter(ObjectInfoHandle);	

		globalData->value = currentValue;
		globalData->minValue = MinValue;
		globalData->maxValue = MaxValue;

		globalData->flags = Flags;
		globalData->values = Values;
		globalData->strings = Strings;

		return globalData;
	}

	void ApplyGlobalData(ObjectGlobalData* globalData) override {
		ObjectGlobalDataCounter* counterData = (ObjectGlobalDataCounter*)globalData;
		
		currentValue = counterData->value;
		MinValue = counterData->minValue;
		MaxValue = counterData->maxValue;
		Flags = counterData->flags;
		Values = counterData->values;
		Strings = counterData->strings;
	}

private:
	int currentValue = 0;
};