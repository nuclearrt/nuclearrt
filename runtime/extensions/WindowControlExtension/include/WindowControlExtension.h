#pragma once

#include "Extension.h"
#include "ObjectInstance.h"
	
class WindowControlExtension : public Extension {
public:
	WindowControlExtension(unsigned int objectInfoHandle, int type, std::string name)
		: Extension(objectInfoHandle, type, name) {}

	CValue GetFrameWinXSize();
	CValue GetFrameWinYSize();
};