#include "KcRuntimeExtension.h"
#include "Application.h"

void KcRuntimeExtension::Initialize()
{
	// NaN
}

bool KcRuntimeExtension::IsRuntimeAnaconda()
{
	return false;
}

bool KcRuntimeExtension::IsRuntimeAndroid()
{
	return false;
}

bool KcRuntimeExtension::IsRuntimeExeStandard()
{
	return false;
}

bool KcRuntimeExtension::IsRuntimeExeHwa()
{
	return false;
}

bool KcRuntimeExtension::IsRuntimeiOS()
{
	return false;
}

bool KcRuntimeExtension::IsRuntimeJava()
{
	return false;
}

bool KcRuntimeExtension::IsRuntimeJavaMobile()
{
	return false;
}

bool KcRuntimeExtension::IsRuntimeMac()
{
	return false;
}

bool KcRuntimeExtension::IsRuntimeSwf()
{
	return false;
}

bool KcRuntimeExtension::IsRuntimeXna()
{
	return false;
}

CValue KcRuntimeExtension::GetRuntimeName()
{
	return CValue("NuclearRT");
}
