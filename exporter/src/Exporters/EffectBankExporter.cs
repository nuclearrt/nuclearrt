using System.Security.Cryptography;
using System.Text;
using CTFAK.MMFParser.EXE.Loaders;
using CTFAK.Utils;

public class EffectBankExporter : BaseExporter
{
	public EffectBankExporter(Exporter exporter) : base(exporter) { }

	public static HashSet<string> thirdPartyShaderHashes = [];

	public static Dictionary<string, HlslToGlsl.Result> translatedShaders = [];

	public override void Export()
	{
		var effectFunctionDefinitions = new StringBuilder();
		var effectFunctions = new StringBuilder();
		translatedShaders.Clear();

		for (int i = 0; i < (GameData.shaders.ShaderList?.Count ?? 0); i++)
		{
			var shader = GameData.shaders.ShaderList.ElementAt(i).Value;
			// translate when any API is missing a hand-written port
			var hash = GetEffectHash(shader);
			if (GetThirdPartyFrag(RuntimeBasePath, "gles300", hash) == null || GetThirdPartyFrag(RuntimeBasePath, "gl330", hash) == null)
				TranslateEffect(shader);

			effectFunctionDefinitions.Append(BuildEffectFunctionDefinition(shader, i));
			effectFunctions.Append(BuildEffectFunction(shader, i));
		}

		thirdPartyShaderHashes = GetThirdPartyShaderHashes(RuntimeBasePath);
		thirdPartyShaderHashes.UnionWith(translatedShaders.Keys);

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

	private static void TranslateEffect(Shader shader)
	{
		var hash = GetEffectHash(shader);
		if (translatedShaders.ContainsKey(hash))
			return;
		if (string.IsNullOrWhiteSpace(shader.Data))
			return;

		try
		{
			var parameterTypes = new Dictionary<string, int>();
			foreach (var parameter in shader.Parameters)
				parameterTypes.TryAdd(parameter.Name, parameter.Type);
			var result = HlslToGlsl.Translate(shader.Data, parameterTypes);
			translatedShaders[hash] = result;
		}
		catch (Exception e)
		{
			Logger.Log($"Effect \"{shader.Name} ({hash})\" could not be translated: {e.Message}");
		}
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

			string valueString = parameter.Value.ToString();
			if (parameter.Type == 1) // float
				valueString = $"{valueString}.0f";
			else if (parameter.Type == 2) // color
				valueString = $"0x{(parameter.Value):X8}";

			result.Append($"        EffectParameter(\"{parameter.Name}\", {parameter.Type}, {valueString})");
			if (i < shader.Parameters.Count - 1) result.AppendLine(",");
			else result.AppendLine();
		}
		result.AppendLine($"    }}, {( (shader.BackgroundTexture & 1) != 0 ? "true" : "false")});");
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
		var hash = GetEffectHash(shader);
		return GetThirdPartyFrag(runtimeBasePath, "gles300", hash) != null || GetThirdPartyFrag(runtimeBasePath, "gl330", hash) != null;
	}

	public HashSet<string> GetThirdPartyShaderHashes(DirectoryInfo runtimeBasePath)
	{
		HashSet<string> hashes = [];
		for (int i = 0; i < (GameData.shaders.ShaderList?.Count ?? 0); i++)
		{
			var shader = GameData.shaders.ShaderList.ElementAt(i).Value;
			if (EffectExists(shader, runtimeBasePath))
				hashes.Add(GetEffectHash(shader));
		}
		return hashes;
	}

	public static FileInfo? GetThirdPartyFrag(DirectoryInfo root, string apiVersion, string shaderHash)
	{
		var specializedRoot = Path.Combine(root.FullName, "shaders", apiVersion, "thirdparty");
		if (!Directory.Exists(specializedRoot))
			return null;
		foreach (var folder in Directory.GetDirectories(specializedRoot))
		{
			if (!Path.GetFileName(folder).Contains($"({shaderHash})", StringComparison.Ordinal))
				continue;
			foreach (var file in Directory.GetFiles(folder))
			{
				if (Path.GetFileName(file).Contains(".frag", StringComparison.OrdinalIgnoreCase))
					return new FileInfo(file);
			}
		}
		return null;
	}
}
