#pragma once

#include <vector>

#include "CValue.h"

class AlterableValues
{
public:
	static const AlterableValues Empty;

	AlterableValues() : Values(std::vector<CValue>(1, CValue(0))) {}
	AlterableValues(const std::vector<CValue>& values) : Values(values) {}

	void SetValue(int index, const CValue& value) { 
		// Resize the vector if needed
		if (index >= Values.size())
			Values.resize(index + 1, CValue(0));
		Values[index] = value;
	}

	void AddValue(int index, const CValue& value) {
		SetValue(index, GetValue(index) + value);
	}
	
	void SubtractValue(int index, const CValue& value) {
		SetValue(index, GetValue(index) - value);
	}

	CValue GetValue(int index) const {
		if (index < 0 || index >= Values.size())
			return CValue(0);
		return Values[index];
	}

	CValue GetValue(const CValue& index) const {
		return GetValue(index.GetIntValue());
	}
private:
	std::vector<CValue> Values;
};

inline const AlterableValues AlterableValues::Empty{};