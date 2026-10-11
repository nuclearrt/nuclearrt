#pragma once

#include "Extension.h"
#include "ObjectInstance.h"
	
class SecToHMSExtension : public Extension {
public:
	SecToHMSExtension(unsigned int objectInfoHandle, int type, std::string name)
		: Extension(objectInfoHandle, type, name) {}

	void Initialize() override;

	// Actions
	void ConvertSeconds(CValue inputSeconds);

	// Expressions
	CValue GetSeconds();
	CValue GetMinutes();
	CValue GetHours();

private:
	int SecondsToConvert;
};