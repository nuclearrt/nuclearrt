#pragma once

#include <vector>
#include <memory>

#include "CValue.h"
#include "CounterBase.h"
#include "ObjectGlobalDataCounter.h"

class Counter : public CounterBase {
public:
	Counter(unsigned int objectInfoHandle, int type, std::string name)
		: CounterBase(objectInfoHandle, type, name) {}
	
	int DefaultValue = 0;
	CValue MinValue = 0;
	CValue MaxValue = 0;

	CValue GetValue() const override { return currentValue; }
	void SetValue(const CValue& value, bool applyBounds = true)
	{
		currentValue = value;
		
		if (applyBounds)
		{
			if (value < MinValue)
			{
				currentValue = MinValue;
			}
			else if (value > MaxValue)
			{
				currentValue = MaxValue;
			}
		}

		collisionBoundsDirty = true;
	}

	void AddValue(const CValue& value)
	{
		SetValue(currentValue + value);
	}

	void SubtractValue(const CValue& value)
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
		collisionBoundsDirty = true; // since we don't call SetValue
	}

private:
	CValue currentValue = 0;
};