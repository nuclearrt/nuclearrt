using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class SetAlterableValueAction : ActionBase
{
	public override int[] ObjectType { get; set; } = [2, 3, 7];
	public override int Num { get; set; } = 31;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new();

		result.AppendLine($"for (ObjectIterator it({GetSelector(eventBase.ObjectInfo, eventBase.ObjectType)}); !it.end(); ++it) {{");
		result.AppendLine($"    auto instance = *it;");
		if (eventBase.Items[0].Loader is ExpressionParameter expressionParameter)
		{
			result.AppendLine($"    (({ExpressionConverter.GetObjectClassName(eventBase.ObjectInfo, eventBase.ObjectType)}*)instance)->Values.SetValue(({ExpressionConverter.ConvertExpression(expressionParameter, eventBase)}).GetIntValue(), {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)});");
		}
		else if (eventBase.Items[0].Loader is Short shortParameter)
		{
			result.AppendLine($"    (({ExpressionConverter.GetObjectClassName(eventBase.ObjectInfo, eventBase.ObjectType)}*)instance)->Values.SetValue({shortParameter.Value}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)});");
		};
		result.AppendLine("}");

		return result.ToString();
	}
}
