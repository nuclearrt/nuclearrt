using CTFAK.Memory;
using CTFAK.CCN.Chunks.Frame;
using System.Text;
using CTFAK.MMFParser.EXE.Loaders.Events.Expressions;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class IntegerSplitExporter : ExtensionExporter
{
	public override string ObjectIdentifier => "ENON";
	public override string ExtensionName => "IntegerSplit";
	public override string CppClassName => "IntegerSplitExtension";

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
				result.AppendLine($"// Integer Split condition {conditionNum} not implemented");
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
				result.AppendLine($"// Integer Split action {actionNum} not implemented");
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
				result = $"{GetExtensionInstance(expression.ObjectInfo, expression.ObjectType)}->LoWord(";
				break;
			case 1:
				result = $"{GetExtensionInstance(expression.ObjectInfo, expression.ObjectType)}->HiWord(";
				break;
			case 2:
				result = $"{GetExtensionInstance(expression.ObjectInfo, expression.ObjectType)}->MakeLong(";
				break;
			default:
				result = $"(/* Integer Split expression {expression.Num} not implemented */";
				break;
		}

		return result;
	}
}
