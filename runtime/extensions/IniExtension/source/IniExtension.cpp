#include "IniExtension.h"
#include "Application.h"
#include <memory>
#include <string>
#include <cstdlib>

#if defined(PLATFORM_WINDOWS)
#include <windows.h>
#include <shlobj.h>
#include <KnownFolders.h>
#endif

void IniExtension::Initialize()
{
	CurrentGroup = "";
	CurrentItem = "";

	SetFileName(Name);
}	

void IniExtension::SetFileName(const std::string& name)
{
	Name = name;
	
	std::filesystem::path path = GetPlatformSaveDirectory();
	if (!std::filesystem::exists(path))
	{
		std::filesystem::create_directories(path);
	}

	path /= Name;

	iniFile = std::make_unique<mINI::INIFile>(path);
	iniFile->read(ini);
}

void IniExtension::SetCurrentGroup(const std::string& group)
{
	CurrentGroup = group;
}

void IniExtension::SetCurrentItem(const std::string& item)
{
	CurrentItem = item;
}

void IniExtension::SetValue(int value)
{
	ini[CurrentGroup][CurrentItem] = std::to_string(value);
	iniFile->write(ini);
}

void IniExtension::SetValue(const std::string& item, int value)
{
	ini[CurrentGroup][item] = std::to_string(value);
	iniFile->write(ini);
}

void IniExtension::SetValue(const std::string& group, const std::string& item, int value)
{
	ini[group][item] = std::to_string(value);
	iniFile->write(ini);
}

void IniExtension::SetString(const std::string& value)
{
	ini[CurrentGroup][CurrentItem] = value;
	iniFile->write(ini);
}

void IniExtension::SetString(const std::string& item, const std::string& value)
{
	ini[CurrentGroup][item] = value;
	iniFile->write(ini);
}

void IniExtension::SetString(const std::string& group, const std::string& item, const std::string& value)
{
	ini[group][item] = value;
	iniFile->write(ini);
}

void IniExtension::SavePosition(ObjectInstance* object)
{
	std::string item = "pos." + object->Name;
	ini[CurrentGroup].set(item, std::to_string(object->X) + "," + std::to_string(object->Y));
	iniFile->write(ini);
}

void IniExtension::LoadPosition(ObjectInstance* object)
{
	std::string item = "pos." + object->Name;
	std::string value = ini[CurrentGroup][item];
	if (value.empty())
	{
		return;
	}

	std::string xValue = value.substr(0, value.find(','));
	std::string yValue = value.substr(value.find(',') + 1);

	object->X = std::stoi(xValue);
	object->Y = std::stoi(yValue);
}

int IniExtension::GetValue()
{
	std::string value = ini[CurrentGroup][CurrentItem];
	return value.empty() ? 0 : std::stoi(value);
}

int IniExtension::GetValue(const std::string& item)
{
	std::string value = ini[CurrentGroup][item];
	return value.empty() ? 0 : std::stoi(value);
}

int IniExtension::GetValue(const std::string& group, const std::string& item)
{
	std::string value = ini[group][item];
	return value.empty() ? 0 : std::stoi(value);
}

std::string IniExtension::GetString()
{
	return ini[CurrentGroup][CurrentItem];
}

std::string IniExtension::GetString(const std::string& item)
{
	return ini[CurrentGroup][item];
}

std::string IniExtension::GetString(const std::string& group, const std::string& item)
{
	return ini[group][item];
}

void IniExtension::DeleteGroup(const std::string& group)
{
	ini.remove(group);
	iniFile->write(ini);
}

void IniExtension::DeleteItem(const std::string& item)
{
	ini[CurrentGroup].remove(item);
	iniFile->write(ini);
}

void IniExtension::DeleteItem(const std::string& group, const std::string& item)
{
	ini[group].remove(item);
	iniFile->write(ini);
}

std::filesystem::path IniExtension::GetPlatformSaveDirectory()
{
#if defined(PLATFORM_WINDOWS)
	PWSTR path_tmp = nullptr;
	HRESULT hres = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &path_tmp);

	if (SUCCEEDED(hres))
	{
		std::wstring appdata_path_w(path_tmp);
		CoTaskMemFree(path_tmp);
		return std::filesystem::path(appdata_path_w) / "NuclearApplications";
    }
	else
	{
		return std::filesystem::path();
	}
#elif defined(PLATFORM_MACOS)
	const char* home = std::getenv("HOME");
	if (home)
	{
		return std::filesystem::path(home) / "Library" / "Application Support" / "NuclearApplications";
	}
	return std::filesystem::path();
#elif defined(PLATFORM_LINUX)
	const char* xdg_data_home = std::getenv("XDG_DATA_HOME");
	if (xdg_data_home)
	{
		return std::filesystem::path(xdg_data_home) / "NuclearApplications";
	}
	
	const char* home = std::getenv("HOME");
	if (home)
	{
		return std::filesystem::path(home) / ".local" / "share" / "NuclearApplications";
	}
	return std::filesystem::path();
#elif defined(PLATFORM_WEB)
	return std::filesystem::path("/disk/AppData/Roaming/NuclearApplications");
#else
	return std::filesystem::path();
#endif
}