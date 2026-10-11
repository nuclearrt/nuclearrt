#include "SecToHMSExtension.h"
#include "Application.h"

void SecToHMSExtension::Initialize()
{
	SecondsToConvert = 0;
}	

void SecToHMSExtension::ConvertSeconds(CValue inputSeconds)
{
    SecondsToConvert = inputSeconds.GetIntValue();

	if (SecondsToConvert < 0)
		SecondsToConvert = 0;
}

CValue SecToHMSExtension::GetSeconds()
{
	return CValue(SecondsToConvert);
}

CValue SecToHMSExtension::GetMinutes()
{
	return CValue(SecondsToConvert / 60);
}

CValue SecToHMSExtension::GetHours()
{
	return CValue(SecondsToConvert / 3600);
}
