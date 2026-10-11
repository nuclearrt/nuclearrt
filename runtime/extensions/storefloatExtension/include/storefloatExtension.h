#pragma once

#include "Extension.h"
#include "ObjectInstance.h"
	
class storefloatExtension : public Extension {
public:
	storefloatExtension(unsigned int objectInfoHandle, int type, std::string name)
		: Extension(objectInfoHandle, type, name) {}

	void Initialize() override;

	// Expressions
	CValue FloatToLong(CValue inputFloat);
	CValue LongToFloat(CValue inputLong);

private:
	// NaN
};