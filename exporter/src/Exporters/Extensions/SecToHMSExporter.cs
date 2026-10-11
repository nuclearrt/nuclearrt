using CTFAK.Memory;
using CTFAK.CCN.Chunks.Frame;
using System.Text;
using CTFAK.MMFParser.EXE.Loaders.Events.Expressions;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class SecToHMSExporter : ExtensionExporter
{
	public override string ObjectIdentifier => "7WWQ"; // Or QWW7
	public override string ExtensionName => "SecToHMS";
	public override string CppClassName => "SecToHMSExtension";

	public override string ExportExtension(byte[] extensionData)
	{
		// No props
	}

	public override string ExportCondition(EventBase eventBase, int conditionNum, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (", bool isGlobal = false)
	{
		StringBuilder result = new();

		switch (conditionNum)
		{
			default:
				result.AppendLine($"// Seconds to HMS condition {conditionNum} not implemented");
				result.AppendLine($"goto {nextLabel};");
				break;
		}

		return result.ToString();
	}

	public override string ExportAction(EventBase eventBase, int actionNum, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, bool isGlobal = false)
	{
		StringBuilder result = new();

		switch (actionNum)
		{
			case 0:
				result.AppendLine($"{ifStatement} {GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->ConvertSeconds({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)})) goto {nextLabel};");
				break;
			default:
				result.AppendLine($"// Seconds to HMS action {actionNum} not implemented");
				break;
		}

		return result.ToString();
	}

	public override string ExportExpression(Expression expression, EventBase eventBase = null)
	{
		string result;

		switch (expression.Num)
		{
			case 0:
				result = $"{GetExtensionInstance(expression.ObjectInfo, expression.ObjectType)}->GetSeconds(";
				break;
			case 1:
				result = $"{GetExtensionInstance(expression.ObjectInfo, expression.ObjectType)}->GetMinutes(";
				break;
			case 2:
				result = $"{GetExtensionInstance(expression.ObjectInfo, expression.ObjectType)}->GetHours(";
				break;
			default:
				result = $"(/* Seconds to HMS expression {expression.Num} not implemented */";
				break;
		}

		return result;
	}
}
