#pragma once

#include "CValue.h"

#include <vector>

class AlterableFlags
{
public:
	static const AlterableFlags Empty;

	AlterableFlags() = default;
	AlterableFlags(const std::vector<bool>& flags) : Flags(flags) {}

	void SetFlag(const CValue& index, bool value) {
		int i = index.GetIntValue();
		// Resize the vector if needed
		if (i >= (int)Flags.size())
			Flags.resize(i + 1, 0);
		Flags[i] = value;
	}

	void ToggleFlag(const CValue& index) {
		SetFlag(index, !GetFlag(index));
	}

	bool GetFlag(const CValue& index) const {
		int i = index.GetIntValue();
		if (i < 0 || i >= (int)Flags.size())
			return false;
		return Flags[i];
	}

	CValue GetFlagValue(const CValue& index) const {
		int i = index.GetIntValue();
		if (i < 0 || i >= (int)Flags.size())
			return CValue(0);
		return Flags[i] ? CValue(1) : CValue(0);
	}
private:
	std::vector<bool> Flags;
};

inline const AlterableFlags AlterableFlags::Empty{};