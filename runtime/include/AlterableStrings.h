#pragma once

#include <vector>
#include <string>

class AlterableStrings
{
public:
	AlterableStrings() = default;
	AlterableStrings(const std::vector<std::string>& values) : Values(values) {}

	void SetString(int index, const std::string& value) {
		// Resize the vector if needed
		if (index >= Values.size())
			Values.resize(index + 1);
		Values[index] = value;
	}

	std::string GetString(int index) const {
		if (index < 0 || index >= Values.size())
			return "";
		return Values[index];
	}
private:
	std::vector<std::string> Values;
};