#pragma once

#include "Application.h"
#include "Extension.h"
#include "ObjectInstance.h"
#include "ArrayData.h"
	
class ArrayExtension : public Extension {
public:
	ArrayExtension(unsigned int objectInfoHandle, int type, std::string name, int dimensionX, int dimensionY, int dimensionZ, int flags)
		: Extension(objectInfoHandle, type, name), Data(new ArrayData(dimensionX, dimensionY, dimensionZ, flags)) {}
	
	ArrayData* Data;
};