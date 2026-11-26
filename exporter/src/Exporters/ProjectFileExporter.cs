using System.IO;
using System.Net;
using System.Text.RegularExpressions;

public class ProjectFileExporter : BaseExporter
{
	public ProjectFileExporter(Exporter exporter) : base(exporter) { }

	public override void Export()
	{
		ExportCMakeLists();
		ExportWindowsResources();
		ExportWebShell();
	}

	void ExportCMakeLists()
	{
		var cmakelists = File.ReadAllText(Path.Combine(RuntimeBasePath.FullName, "CMakeLists.txt"));
		cmakelists = Regex.Replace(cmakelists, @"project\(\S+", "project(" + Config.ExecutableName);
		cmakelists = SetCMakeVariable(cmakelists, "FRIENDLY_NAME", GameData.name ?? "");
		cmakelists = SetCMakeVariable(cmakelists, "DOMAIN_NAME", Config.PackageName);
		cmakelists = SetCMakeVariable(cmakelists, "APP_VERSION", Config.Version);
		cmakelists = SetCMakeVariable(cmakelists, "APP_AUTHOR", GameData.author ?? "");
		cmakelists = SetCMakeVariable(cmakelists, "APP_COPYRIGHT", GameData.copyright ?? "");
		SaveFile(Path.Combine(OutputPath.FullName, "CMakeLists.txt"), cmakelists);
	}

	static string SetCMakeVariable(string cmakelists, string name, string value)
	{
		string escaped = value.Replace("\\", "\\\\").Replace("\"", "\\\"").Replace("$", "\\$").Replace(";", "\\;");
		return Regex.Replace(cmakelists, $@"set\({name} ""[^""]*""\)", _ => $"set({name} \"{escaped}\")");
	}

	void ExportWindowsResources()
	{
		string v = string.Join(",", Config.NumericVersion);
		string Rc(string s) => s.Replace("\"", "\"\"").Replace("\r", "").Replace("\n", " ");

		string rc = $@"#pragma code_page(65001)
#include <winver.h>

IDI_ICON1 ICON DISCARDABLE ""icons/windows.ico""

VS_VERSION_INFO VERSIONINFO
FILEVERSION {v}
PRODUCTVERSION {v}
FILEFLAGSMASK 0x3fL
FILEFLAGS 0x0L
FILEOS VOS_NT_WINDOWS32
FILETYPE VFT_APP
FILESUBTYPE VFT2_UNKNOWN
BEGIN
	BLOCK ""StringFileInfo""
	BEGIN
		BLOCK ""040904b0""
		BEGIN
			VALUE ""CompanyName"", ""{Rc(GameData.author ?? "")}""
			VALUE ""FileDescription"", ""{Rc(GameData.name)}""
			VALUE ""FileVersion"", ""{string.Join(".", Config.NumericVersion)}""
			VALUE ""InternalName"", ""{Rc(Config.ExecutableName)}""
			VALUE ""LegalCopyright"", ""{Rc(GameData.copyright ?? "")}""
			VALUE ""OriginalFilename"", ""{Rc(Config.ExecutableName)}.exe""
			VALUE ""ProductName"", ""{Rc(GameData.name)}""
			VALUE ""ProductVersion"", ""{string.Join(".", Config.NumericVersion)}""
		END
	END
	BLOCK ""VarFileInfo""
	BEGIN
		VALUE ""Translation"", 0x409, 1200
	END
END
";
		SaveFile(Path.Combine(OutputPath.FullName, "resources.rc"), rc);
	}

	void ExportWebShell()
	{
		var shell = File.ReadAllText(Path.Combine(RuntimeBasePath.FullName, "web_shell.html"));
		shell = Regex.Replace(shell, "<title>.*?</title>", _ => $"<title>{WebUtility.HtmlEncode(GameData.name)}</title>");
		SaveFile(Path.Combine(OutputPath.FullName, "web_shell.html"), shell);
	}
}
