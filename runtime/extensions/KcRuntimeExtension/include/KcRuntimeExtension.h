#pragma once

#include "Extension.h"
#include "ObjectInstance.h"
	
class KcRuntimeExtension : public Extension {
public:
	KcRuntimeExtension(unsigned int objectInfoHandle, int type, std::string name)
		: Extension(objectInfoHandle, type, name) {}

	void Initialize() override;

	// Conditions
	bool IsRuntimeAnaconda();
	bool IsRuntimeAndroid();
	bool IsRuntimeExeStandard();
	bool IsRuntimeExeHwa();
	bool IsRuntimeiOS();
	bool IsRuntimeJava();
	bool IsRuntimeJavaMobile();
	bool IsRuntimeMac();
	bool IsRuntimeSwf();
	bool IsRuntimeXna();

	// Expressions
	CValue GetRuntimeName();

private:
	// NaN
};