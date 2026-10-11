#pragma once

#include "Extension.h"
#include "ObjectInstance.h"
	
class FModObjectExtension : public Extension {
public:
	FModObjectExtension(unsigned int objectInfoHandle, int type, std::string name)
		: Extension(objectInfoHandle, type, name) {}

	void Initialize() override;

	// Expressions
	CValue FloatModulus(CValue a, CValue b);

private:
	// NaN
};