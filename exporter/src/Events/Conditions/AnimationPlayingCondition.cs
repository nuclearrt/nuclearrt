using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class AnimationPlayingCondition : ConditionBase
{
	public override int[] ObjectType { get; set; } = [2];
	public override int Num { get; set; } = -3;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new();

		string sequence;
		if (eventBase.Items[0].Loader is ExpressionParameter)
			sequence = $"(({ConvertExpression(eventBase, 0)}) - 1)";
		else
			sequence = ((Short)eventBase.Items[0].Loader).Value.ToString();

		result.AppendLine($"for (ObjectIterator it({GetSelector(eventBase.ObjectInfo, eventBase.ObjectType)}); !it.end(); ++it) {{");
		result.AppendLine($"    auto instance = *it;");
		result.AppendLine($"    {ifStatement}((Active*)instance)->animations.IsSequencePlaying({sequence})) it.deselect();");
		result.AppendLine("}");

		result.AppendLine($"if ({GetSelector(eventBase.ObjectInfo, eventBase.ObjectType)}.Count() == 0) goto {nextLabel};");

		return result.ToString();
	}
}
