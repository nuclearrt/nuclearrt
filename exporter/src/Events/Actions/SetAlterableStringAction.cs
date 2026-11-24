using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class SetAlterableStringAction : ActionBase
{
	public override int[] ObjectType { get; set; } = [2, 3, 7];
	public override int Num { get; set; } = 49;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new();

		result.AppendLine($"for (ObjectIterator it({GetSelector(eventBase.ObjectInfo, eventBase.ObjectType)}); !it.end(); ++it) {{");
		result.AppendLine($"    auto instance = *it;");
		if (eventBase.Items[0].Loader is ExpressionParameter expressionParameter)
		{
			result.AppendLine($"    (({ExpressionConverter.GetObjectClassName(eventBase.ObjectInfo, eventBase.ObjectType)}*)instance)->Strings.SetString(({ExpressionConverter.ConvertExpression(expressionParameter, eventBase)}).GetIntValue(), ({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)}).GetStringValue());");
		}
		else if (eventBase.Items[0].Loader is Short shortParameter)
		{
			result.AppendLine($"    (({ExpressionConverter.GetObjectClassName(eventBase.ObjectInfo, eventBase.ObjectType)}*)instance)->Strings.SetString({shortParameter.Value}, ({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)}).GetStringValue());");
		};
		result.AppendLine("}");

		return result.ToString();
	}
}
