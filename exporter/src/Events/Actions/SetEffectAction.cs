using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class SetEffectAction : ActionBase
{
	public override int[] ObjectType { get; set; } = [2, 3, 7];
	public override int Num { get; set; } = 63;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new();

		string effectName = ((StringParam)eventBase.Items[0].Loader).Value;

		result.AppendLine($"for (ObjectIterator it(*{GetSelector(eventBase.ObjectInfo, eventBase.ObjectType)}); !it.end(); ++it) {{");
		result.AppendLine($"	auto instance = *it;");
		result.AppendLine($"	instance->effectInstance = nullptr;");

		if (string.IsNullOrEmpty(effectName))
		{
			result.AppendLine($"	instance->Effect = 0;");
		}
		else if (effectName == "Invert")
		{
			result.AppendLine($"	instance->Effect = 2;");
		}
		else if (effectName == "Add")
		{
			result.AppendLine($"	instance->Effect = 9;");
		}
		else if (effectName == "Mono")
		{
			result.AppendLine($"	instance->Effect = 10;");
		}
		else if (effectName == "Sub")
		{
			result.AppendLine($"	instance->Effect = 11;");
		}
		else
		{
			var shader = Exporter.Instance.GameData.shaders.ShaderList.Values.FirstOrDefault(s => s.Name == effectName);
			if (shader != null)
			{
				int id = shader != null ? Exporter.Instance.GameData.shaders.ShaderList.FirstOrDefault(x => x.Value == shader).Key : -1;
				result.AppendLine($"	instance->effectInstance = EffectBank::CreateEffect_{StringUtils.SanitizeObjectName(effectName)}_{id}();");
			}
		}

		result.AppendLine("}");

		return result.ToString();
	}
}
