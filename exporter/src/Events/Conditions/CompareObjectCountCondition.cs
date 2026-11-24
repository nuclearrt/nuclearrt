using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class CompareObjectCountCondition : ConditionBase
{
	public override int[] ObjectType { get; set; } = [2, 3, 7];
	public override int Num { get; set; } = -32;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"{ifStatement} (CValue({ExpressionConverter.GetSelector(eventBase.ObjectInfo, eventBase.ObjectType)}.Size()) {ExpressionConverter.GetComparisonSymbol(((ExpressionParameter)eventBase.Items[0].Loader).Comparsion)} {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)})) goto {nextLabel};";
	}
}
