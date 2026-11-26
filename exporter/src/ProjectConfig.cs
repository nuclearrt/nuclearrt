using System.Text.RegularExpressions;
using CTFAK.CCN;
using CTFAK.MFA;
using Newtonsoft.Json;

public class ProjectConfig
{
	public const string SidecarExtension = ".nuclearrt.json";

	public string ExecutableName { get; set; } = "";
	public string PackageName { get; set; } = "";
	public string Version { get; set; } = "1.0.0.0";

	public static ProjectConfig Load(string mfaPath, GameData gameData, MFAData mfaData)
	{
		ProjectConfig config = new();

		string sidecarPath = Path.ChangeExtension(mfaPath, SidecarExtension);
		if (File.Exists(sidecarPath))
		{
			try
			{
				config = JsonConvert.DeserializeObject<ProjectConfig>(File.ReadAllText(sidecarPath)) ?? new();
			}
			catch (JsonException ex)
			{
				throw new Exception($"Invalid project config {sidecarPath}: {ex.Message}", ex);
			}
		}

		//pick from config or default to best guess
		config.ExecutableName = Regex.Replace(Pick(config.ExecutableName, StringUtils.SanitizeObjectName(gameData.name)), @"[^A-Za-z0-9_.+-]", "_");
		config.PackageName = Pick(config.PackageName, $"com.{StringUtils.SanitizeObjectName(gameData.author).ToLower().Replace("_", "")}.{StringUtils.SanitizeObjectName(gameData.name).ToLower().Replace("_", "")}");
		config.Version = Pick(config.Version, "1.0.0.0");

		return config;
	}

	[JsonIgnore]
	public int[] NumericVersion
	{
		get
		{
			int[] parts = new int[4];
			string[] split = Version.Split('.');
			for (int i = 0; i < Math.Min(4, split.Length); i++)
			{
				Match m = Regex.Match(split[i], @"\d+");
				if (m.Success) int.TryParse(m.Value, out parts[i]);
			}
			return parts;
		}
	}

	static string Pick(params string?[] values)
	{
		foreach (string? value in values)
		{
			if (!string.IsNullOrWhiteSpace(value)) return value.Trim();
		}
		return "";
	}
}
