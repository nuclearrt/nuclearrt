using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class SetScaleAction : ActionBase
{
	public override int[] ObjectType { get; set; } = [2];
	public override int Num { get; set; } = 85;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new StringBuilder();

		result.AppendLine($"for (ObjectIterator it(*{GetSelector(eventBase.ObjectInfo, eventBase.ObjectType)}); !it.end(); ++it) {{");
		result.AppendLine($"    auto instance = *it;");
		string scale = "";
		if (eventBase.Items[0].Loader is ExpressionParameter expressionParameter) {
			scale = ExpressionConverter.ConvertExpression(expressionParameter, eventBase);
		}
		result.AppendLine($"    ((Active*)instance)->SetXScale({scale});");
		result.AppendLine($"    ((Active*)instance)->SetYScale({scale});");
		result.AppendLine("}");

		return result.ToString();
	}
}

public class SetXScaleAction : ActionBase
{
	public override int[] ObjectType { get; set; } = [2];
	public override int Num { get; set; } = 86;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new StringBuilder();

		result.AppendLine($"for (ObjectIterator it(*{GetSelector(eventBase.ObjectInfo, eventBase.ObjectType)}); !it.end(); ++it) {{");
		result.AppendLine($"    auto instance = *it;");
		result.AppendLine($"    ((Active*)instance)->SetXScale({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
		result.AppendLine("}");

		return result.ToString();
	}
}

public class SetYScaleAction : ActionBase
{
	public override int[] ObjectType { get; set; } = [2];
	public override int Num { get; set; } = 87;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new StringBuilder();

		result.AppendLine($"for (ObjectIterator it(*{GetSelector(eventBase.ObjectInfo, eventBase.ObjectType)}); !it.end(); ++it) {{");
		result.AppendLine($"    auto instance = *it;");
		result.AppendLine($"    ((Active*)instance)->SetYScale({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
		result.AppendLine("}");

		return result.ToString();
	}
}

