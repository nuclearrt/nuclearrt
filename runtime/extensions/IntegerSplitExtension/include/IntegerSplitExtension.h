#pragma once

#include "Extension.h"
#include "ObjectInstance.h"
	
class IntegerSplitExtension : public Extension {
public:
	IntegerSplitExtension(unsigned int objectInfoHandle, int type, std::string name)
		: Extension(objectInfoHandle, type, name) {}

	void Initialize() override;

	// Expressions
	CValue LoWord(CValue inputValue);
	CValue HiWord(CValue inputValue);
	CValue MakeLong(CValue inputLow, CValue inputHigh);

private:
	// NaN
};