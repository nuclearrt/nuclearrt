using CTFAK.Memory;
using CTFAK.CCN.Chunks.Frame;
using System.Text;
using CTFAK.MMFParser.EXE.Loaders.Events.Expressions;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class storefloatExporter : ExtensionExporter
{
	public override string ObjectIdentifier => "lfts"; // Or stfl
	public override string ExtensionName => "storefloat";
	public override string CppClassName => "storefloatExtension";

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
				result.AppendLine($"// storefloat condition {conditionNum} not implemented");
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
			default:
				result.AppendLine($"// storefloat action {actionNum} not implemented");
				break;
		}

		return result.ToString();
	}

	public override string ExportExpression(Expression expression, EventBase eventBase = null)
	{
		string result;

		switch (expression.Num)
		{
			case 0: // Float to Long
				result = $"{GetExtensionInstance(expression.ObjectInfo, expression.ObjectType)}->FloatToLong(";
				break;
			case 1: // Long to Float
				result = $"{GetExtensionInstance(expression.ObjectInfo, expression.ObjectType)}->LongToFloat(";
				break;
			default:
				result = $"(/* storefloat expression {expression.Num} not implemented */";
				break;
		}

		return result;
	}
}
