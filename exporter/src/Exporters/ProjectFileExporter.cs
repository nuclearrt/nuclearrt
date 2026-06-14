using System.IO;

public class ProjectFileExporter : BaseExporter
{
	public ProjectFileExporter(Exporter exporter) : base(exporter) { }

	public override void Export()
	{
		var cmakelistsPath = Path.Combine(RuntimeBasePath.FullName, "CMakeLists.txt");
		var cmakelists = File.ReadAllText(cmakelistsPath);
		cmakelists = cmakelists.Replace("nuclearrt-runtime", SanitizeObjectName(GameData.name));
		cmakelists = cmakelists.Replace("NuclearRT-Runtime", GameData.name);
		cmakelists = cmakelists.Replace("com.nuclearrt.runtime", $"com.nuclearrt.{SanitizeObjectName(GameData.name).ToLower().Replace("_", "")}"); //TODO: config to allow custom package name
		SaveFile(Path.Combine(OutputPath.FullName, "CMakeLists.txt"), cmakelists);
	}
}
