using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class AlterableValueComparisonCondition : ConditionBase
{
	public override int[] ObjectType { get; set; } = [2, 3, 7];
	public override int Num { get; set; } = -27;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new();

		result.AppendLine($"for (ObjectIterator it({GetSelector(eventBase.ObjectInfo, eventBase.ObjectType)}); !it.end(); ++it) {{");
		result.AppendLine($"    auto instance = *it;");
		if (eventBase.Items[0].Loader is ExpressionParameter expressionParameter)
		{
			result.AppendLine($"    if ((({ExpressionConverter.GetObjectClassName(eventBase.ObjectInfo, eventBase.ObjectType)}*)instance)->Values.GetValue({ExpressionConverter.ConvertExpression(expressionParameter, eventBase)}) {ExpressionConverter.GetOppositeComparison(((ExpressionParameter)eventBase.Items[1].Loader).Comparsion)} {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)}) it.deselect();");
		}
		else if (eventBase.Items[0].Loader is Short shortParameter)
		{
			result.AppendLine($"    if ((({ExpressionConverter.GetObjectClassName(eventBase.ObjectInfo, eventBase.ObjectType)}*)instance)->Values.GetValue({shortParameter.Value}) {ExpressionConverter.GetOppositeComparison(((ExpressionParameter)eventBase.Items[1].Loader).Comparsion)} {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)}) it.deselect();");
		}
		result.AppendLine("}");

		//If no instances are selected, we go to the end label
		result.AppendLine($"if ({GetSelector(eventBase.ObjectInfo, eventBase.ObjectType)}.Count() == 0) goto {nextLabel};");

		return result.ToString();
	}
}
