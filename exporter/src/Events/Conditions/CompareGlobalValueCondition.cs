using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class CompareGlobalValueCondition : ConditionBase
{
	public override int[] ObjectType { get; set; } = [-1];
	public override int Num { get; set; } = -8;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		if (eventBase.Items[0].Loader is GlobalValue globalValue)
		{
			return $"{ifStatement} (Application::Instance().GetAppData()->GetGlobalValue({globalValue.Value}) {ExpressionConverter.GetComparisonSymbol(((ExpressionParameter)eventBase.Items[1].Loader).Comparsion)} {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)})) goto {nextLabel};";
		}
		if (eventBase.Items[0].Loader is ExpressionParameter expressionParameter)
		{
			return $"{ifStatement} ((Application::Instance().GetAppData()->GetGlobalValue({ExpressionConverter.ConvertExpression(expressionParameter, eventBase)} - 1) {ExpressionConverter.GetComparisonSymbol(((ExpressionParameter)eventBase.Items[1].Loader).Comparsion)} {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)}))) goto {nextLabel};"; // -1 since GetGlobalValue is 0-indexed
		}
		return "";
	}
}
