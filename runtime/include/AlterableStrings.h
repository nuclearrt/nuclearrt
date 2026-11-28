#pragma once

#include <vector>
#include <string>

#include "CValue.h"

class AlterableStrings
{
public:
	static const AlterableStrings Empty;

	AlterableStrings() = default;
	AlterableStrings(const std::vector<std::string>& values) : Values(values) {}

	void SetString(int index, const std::string& value) {
		// Resize the vector if needed
		if (index >= Values.size())
			Values.resize(index + 1);
		Values[index] = value;
	}

	CValue GetString(int index) const {
		if (index < 0 || index >= Values.size())
			return CValue("");
		return CValue(Values[index]);
	}

	CValue GetString(const CValue& index) const {
		return GetString(index.GetIntValue());
	}
private:
	std::vector<std::string> Values;
};

inline const AlterableStrings AlterableStrings::Empty{};