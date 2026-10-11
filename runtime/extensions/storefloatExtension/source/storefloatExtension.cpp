#include "storefloatExtension.h"
#include "Application.h"

void storefloatExtension::Initialize()
{
	// NaN
}	

CValue storefloatExtension::FloatToLong(CValue inputFloat)
{
	return CValue((int)((float)(inputFloat.GetDoubleValue())));
}

CValue storefloatExtension::LongToFloat(CValue inputLong)
{
	return CValue((float)(inputLong.GetIntValue()));
}
