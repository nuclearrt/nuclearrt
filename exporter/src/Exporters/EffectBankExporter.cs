using System.Security.Cryptography;
using System.Text;
using CTFAK.MMFParser.EXE.Loaders;
using CTFAK.Utils;

public class EffectBankExporter : BaseExporter
{
	public EffectBankExporter(Exporter exporter) : base(exporter) { }

	public static Dictionary<string, FileInfo> thirdPartyShaders = [];

	public override void Export()
	{
		var effectFunctionDefinitions = new StringBuilder();
		var effectFunctions = new StringBuilder();

		for (int i = 0; i < (GameData.shaders.ShaderList?.Count ?? 0); i++)
		{
			var shader = GameData.shaders.ShaderList.ElementAt(i).Value;
			if (!EffectExists(shader, RuntimeBasePath))
			{
				Logger.Log($"Effect \"{shader.Name} ({GetEffectHash(shader)})\" does not exist!");
			}

			effectFunctionDefinitions.Append(BuildEffectFunctionDefinition(shader, i));
			effectFunctions.Append(BuildEffectFunction(shader, i));
		}

		thirdPartyShaders = GetAllUsedThirdPartyShaders(RuntimeBasePath);

		var effectBankListPath = Path.Combine(RuntimeBasePath.FullName, "source", "EffectBank.template.cpp");
		var effectBankList = File.ReadAllText(effectBankListPath);
		effectBankList = effectBankList.Replace("{{ EFFECT_FUNCTIONS }}", effectFunctions.ToString());
		SaveFile(Path.Combine(OutputPath.FullName, "source", "EffectBank.cpp"), effectBankList.ToString());
		File.Delete(Path.Combine(OutputPath.FullName, "source", "EffectBank.template.cpp"));

		var effectBankIncludePath = Path.Combine(OutputPath.FullName, "include", "EffectBank.template.h");
		var effectBankInclude = File.ReadAllText(effectBankIncludePath);
		effectBankInclude = effectBankInclude.Replace("{{ EFFECT_DEFINITIONS }}", effectFunctionDefinitions.ToString());
		SaveFile(Path.Combine(OutputPath.FullName, "include", "EffectBank.h"), effectBankInclude);
		File.Delete(effectBankIncludePath);
	}

	private string BuildEffectFunctionDefinition(Shader shader, int index)
	{
		return $"static EffectInstance* CreateEffect_{StringUtils.SanitizeObjectName(shader.Name)}_{index}();\n";
	}

	private string BuildEffectFunction(Shader shader, int index)
	{
		StringBuilder result = new StringBuilder();
		result.AppendLine($"EffectInstance* EffectBank::CreateEffect_{StringUtils.SanitizeObjectName(shader.Name)}_{index}() {{");
		result.AppendLine($"    return new EffectInstance(\"{GetEffectHash(shader)}\", {{");
		for (int i = 0; i < shader.Parameters.Count; i++)
		{
			var parameter = shader.Parameters[i];
			result.Append($"        EffectParameter(\"{parameter.Name}\", {parameter.Type}, {parameter.Value})");
			if (i < shader.Parameters.Count - 1) result.AppendLine(",");
			else result.AppendLine();
		}
		result.AppendLine($"    }});");
		result.AppendLine($"}}");

		return result.ToString();
	}

	public static string GetEffectHash(Shader shader)
	{
		var hash = MD5.HashData(Encoding.UTF8.GetBytes(shader.Data));
		//get first 8 characters
		return Convert.ToHexStringLower(hash)[..8];
	}

	public static bool EffectExists(Shader shader, DirectoryInfo runtimeBasePath)
	{
		var shaderFolder = Path.Combine(runtimeBasePath.FullName, "shaders", "thirdparty");
		if (!Directory.Exists(shaderFolder)) return false;
		foreach (var folder in Directory.GetDirectories(shaderFolder))
		{
			if (Path.GetFileName(folder).Contains($"({GetEffectHash(shader)})")) return true;
		}
		return false;
	}

	public Dictionary<string, FileInfo> GetAllUsedThirdPartyShaders(DirectoryInfo runtimeBasePath)
	{
		Dictionary<string, FileInfo> shaders = [];
		var shaderFolder = Path.Combine(runtimeBasePath.FullName, "shaders", "thirdparty");
		if (!Directory.Exists(shaderFolder)) return [];
		for (int i = 0; i < (GameData.shaders.ShaderList?.Count ?? 0); i++)
		{
			var shader = GameData.shaders.ShaderList.ElementAt(i).Value;
			string shaderHash = GetEffectHash(shader);
			foreach (var folder in Directory.GetDirectories(shaderFolder))
			{
				if (Path.GetFileName(folder).Contains($"({shaderHash})"))
				{
					foreach (var file in Directory.GetFiles(folder))
					{
						//get first frag shader
						if (Path.GetFileName(file).Contains(".frag"))
						{
							shaders.Add(shaderHash, new FileInfo(file));
							break;
						}
						else
						{
							Logger.Log($"Unknown file in shader folder: {file}");
						}
					}
				}
			}
		}
		return shaders;
	}
}
