#pragma once

#include <vector>

class AlterableFlags
{
public:
	AlterableFlags() = default;
	AlterableFlags(const std::vector<bool>& flags) : Flags(flags) {}

	void SetFlag(int index, bool value) {
		// Resize the vector if needed
		if (index >= Flags.size())
			Flags.resize(index + 1, 0);
		Flags[index] = value;
	}

	void ToggleFlag(int index) {
		SetFlag(index, !GetFlag(index));
	}

	bool GetFlag(int index) const {
		if (index < 0 || index >= Flags.size())
			return false;
		return Flags[index];
	}

	int GetFlagValue(int index) const {
		if (index < 0 || index >= Flags.size())
			return 0;
		return Flags[index] ? 1 : 0;
	}
private:
	std::vector<bool> Flags;
};