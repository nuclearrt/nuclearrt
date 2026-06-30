using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class XComparisonCondition : ConditionBase
{
	public override int[] ObjectType { get; set; } = [2, 7];
	public override int Num { get; set; } = -17;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new();

		result.AppendLine($"for (ObjectIterator it(*{GetSelector(eventBase.ObjectInfo, eventBase.ObjectType)}); !it.end(); ++it) {{");
		result.AppendLine($"    auto instance = *it;");
		result.AppendLine($"    if ({(eventBase.Num == -17 ? "instance->GetX()" : "instance->GetY()")} {ExpressionConverter.GetOppositeComparison(((ExpressionParameter)eventBase.Items[0].Loader).Comparsion)} {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}) it.deselect();");
		result.AppendLine("}");

		//If no instances are selected, we go to the end label
		result.AppendLine($"if ({GetSelector(eventBase.ObjectInfo, eventBase.ObjectType)}->Count() == 0) goto {nextLabel};");

		return result.ToString();
	}
}

public class YComparisonCondition : XComparisonCondition
{
	public override int Num { get; set; } = -16;
}
