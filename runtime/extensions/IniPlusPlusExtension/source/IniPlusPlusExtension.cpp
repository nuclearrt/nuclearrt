#include "IniPlusPlusExtension.h"
#include "Application.h"
#include <memory>
#include <string>
#include <cstdlib>

std::unordered_map<std::string, INIInstance*> IniPlusPlusExtension::globalDataMap;

#if defined(PLATFORM_WINDOWS)
#include <windows.h>
#include <shlobj.h>
#include <KnownFolders.h>
#endif

void IniPlusPlusExtension::Initialize()
{
	if (GlobalData)
	{
		auto it = globalDataMap.find(GlobalDataKey);
		if (it != globalDataMap.end())
		{
			currentData = it->second;
		}
		else
		{
			currentData = new INIInstance();
			globalDataMap[GlobalDataKey] = currentData;
		}
	}

	if (DefaultFile) {
		std::filesystem::path defaultPath(DefaultFilePath);
		std::string filename = defaultPath.filename().string();
		std::string subDir = defaultPath.has_parent_path() ? defaultPath.parent_path().string() : std::string();

		std::filesystem::path saveDir = GetBaseSaveDirectory(BaseFolder, subDir);
		if (!saveDir.empty() && !std::filesystem::exists(saveDir) && CreateFolders) {
			std::filesystem::create_directories(saveDir);
		}

		currentData = new INIInstance();
		currentData->FilePath = (saveDir / filename).string();
		currentData->ReadOnly = ReadOnly;
		currentData->Encrypted = UseEncryption;
		currentData->Compressed = UseCompression;
		currentData->EncryptionKey = EncryptionKey;
		currentData->NewLineText = NewLine ? NewLineText : "\n";
		currentData->CaseSensitive = CaseSensitive;
		currentData->UndoBufferSize = UndoBufferSize;
		currentData->RedoBufferSize = RedoBufferSize;

		if (!InitialData.empty()) {
			currentData->ReadIniData(InitialData);
		}
		
		currentData->LoadIniFromPath(currentData->FilePath);
	}
}

void IniPlusPlusExtension::Update(float deltaTime)
{
	if (EnableAutoSave && currentData && currentData->dirty) {
		currentData->SaveINI();
	}
}

void IniPlusPlusExtension::SetCurrentGroup(const std::string &group)
{
	CurrentGroup = group;
}

void IniPlusPlusExtension::SetCurrentGroupItemValue(const std::string &item, int type, double value)
{
	SetItemValue(CurrentGroup, item, type, value);
}

void IniPlusPlusExtension::SetCurrentGroupItemString(const std::string &item, const std::string &value)
{
	SetItemString(CurrentGroup, item, value);
}

void IniPlusPlusExtension::SetItemValue(const std::string &group, const std::string &item, int type, double value)
{
	if (!currentData) return;
	
	if (type == 0) // int
		currentData->SetValue(group, item, std::to_string(static_cast<int>(value)));
	else if (type == 1) // double
		currentData->SetValue(group, item, std::to_string(static_cast<double>(value)));
}

void IniPlusPlusExtension::SetItemString(const std::string &group, const std::string &item, const std::string &value)
{
	if (!currentData) return;
	
	currentData->SetValue(group, item, value);
}

void IniPlusPlusExtension::DeleteCurrentGroup()
{
	DeleteGroup(CurrentGroup);
}

void IniPlusPlusExtension::DeleteGroup(const std::string &group)
{
	if (!currentData) return;
	currentData->DeleteGroup(group);
}

void IniPlusPlusExtension::DeleteCurrentGroupItem(const std::string &item)
{
	DeleteGroupItem(CurrentGroup, item);
}

void IniPlusPlusExtension::DeleteGroupItem(const std::string &group, const std::string &item)
{
	if (!currentData) return;
	currentData->DeleteGroupItem(group, item);
}

void IniPlusPlusExtension::ClearINI()
{
	if (!currentData) return;
	currentData->Clear();
}

void IniPlusPlusExtension::Save()
{
	if (currentData) currentData->SaveINI();
}

bool IniPlusPlusExtension::CurrentGroupExists() const
{
	return GroupExists(CurrentGroup);
}

bool IniPlusPlusExtension::GroupExists(const std::string &group) const
{
	if (!currentData) return false;
	return currentData->GroupExists(group);
}

bool IniPlusPlusExtension::CurrentGroupItemExists(const std::string &item) const
{
	return GroupItemExists(CurrentGroup, item);
}

bool IniPlusPlusExtension::GroupItemExists(const std::string &group, const std::string &item) const
{
	if (!currentData) return false;
	return currentData->GroupItemExists(group, item);
}

int IniPlusPlusExtension::GetCurrentGroupItemValue(const std::string &item, int defaultValue)
{
	return GetItemValue(CurrentGroup, item, defaultValue);
}

std::string IniPlusPlusExtension::GetCurrentGroupItemString(const std::string &item, const std::string &defaultValue)
{
	return GetItemString(CurrentGroup, item, defaultValue);
}

int IniPlusPlusExtension::GetItemValue(const std::string &group, const std::string &item, int defaultValue)
{
	if (!currentData) return defaultValue;

	std::string valueStr = currentData->GetValue(group, item, std::to_string(defaultValue));
	return std::stoi(valueStr);
}

std::string IniPlusPlusExtension::GetItemString(const std::string &group, const std::string &item, const std::string &defaultValue)
{
	if (!currentData) return defaultValue;

	return currentData->GetValue(group, item, defaultValue);
}


std::filesystem::path IniPlusPlusExtension::GetBaseSaveDirectory(int8_t baseFolder, const std::string &defaultFilePath)
{
#if defined(PLATFORM_WINDOWS)
	PWSTR path_tmp = nullptr;
	HRESULT hres = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &path_tmp);

	if (SUCCEEDED(hres))
	{
		std::wstring appdata_path_w(path_tmp);
		CoTaskMemFree(path_tmp);
		return std::filesystem::path(appdata_path_w) / "NuclearApplications" / defaultFilePath;
	}
	else
	{
		return std::filesystem::path();
	}
#elif defined(PLATFORM_MACOS)
	const char* home = std::getenv("HOME");
	if (home)
	{
		return std::filesystem::path(home) / "Library" / "Application Support" / "NuclearApplications" / defaultFilePath;
	}
	return std::filesystem::path();
#elif defined(PLATFORM_LINUX)
	const char* xdg_data_home = std::getenv("XDG_DATA_HOME");
	if (xdg_data_home)
	{
		return std::filesystem::path(xdg_data_home) / "NuclearApplications" / defaultFilePath;
	}
	
	const char* home = std::getenv("HOME");
	if (home)
	{
		return std::filesystem::path(home) / ".local" / "share" / "NuclearApplications" / defaultFilePath;
	}
	return std::filesystem::path();
#elif defined(PLATFORM_WEB)
	return std::filesystem::path("/disk/AppData/Roaming/NuclearApplications") / defaultFilePath;
#else
	return std::filesystem::path();
#endif
}