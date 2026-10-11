#pragma once

#include "Extension.h"
#include "ObjectInstance.h"
	
class commentorExtension : public Extension {
public:
	commentorExtension(unsigned int objectInfoHandle, int type, std::string name)
		: Extension(objectInfoHandle, type, name) {}

	void Initialize() override;

	// Actions
	void ActComment();

	// Conditions
	bool CndComment();

private:
	// NaN
};