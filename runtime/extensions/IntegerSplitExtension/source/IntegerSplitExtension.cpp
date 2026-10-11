#include "IntegerSplitExtension.h"
#include "Application.h"

void IntegerSplitExtension::Initialize()
{
	// NaN
}	

CValue IntegerSplitExtension::LoWord(CValue inputValue)
{
	return CValue(inputValue.GetIntValue() & 0xFFFF);
}

CValue IntegerSplitExtension::HiWord(CValue inputValue)
{
	return CValue((inputValue.GetIntValue() >> 16) & 0xFFFF);
}

CValue IntegerSplitExtension::MakeLong(CValue inputLow, CValue inputHigh)
{
	return CValue((inputLow.GetIntValue() & 0xFFFF) | ((inputHigh.GetIntValue() & 0xFFFF) << 16));
}
