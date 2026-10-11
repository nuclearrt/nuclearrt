#include "FModObjectExtension.h"
#include "Application.h"

void FModObjectExtension::Initialize()
{
	// NaN
}	

CValue FModObjectExtension::FloatModulus(CValue a, CValue b)
{
	return CValue(a.GetDoubleValue() % b.GetDoubleValue());
}
