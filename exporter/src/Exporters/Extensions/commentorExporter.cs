using CTFAK.Memory;
using CTFAK.CCN.Chunks.Frame;
using System.Text;
using CTFAK.MMFParser.EXE.Loaders.Events.Expressions;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class commentorExporter : ExtensionExporter
{
	public override string ObjectIdentifier => "1MOC";
	public override string ExtensionName => "commentor";
	public override string CppClassName => "commentorExtension";

	public override string ExportExtension(byte[] extensionData)
	{
		// No props
	}

	public override string ExportCondition(EventBase eventBase, int conditionNum, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (", bool isGlobal = false)
	{
		StringBuilder result = new();

		switch (conditionNum)
		{
			case 0:
				result.AppendLine($"{ifStatement} {GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->CndComment()) goto {nextLabel};");
				break;
			default:
				result.AppendLine($"// Advanced Comment object condition {conditionNum} not implemented");
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
			 	result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->ActComment();");
			 	break;
			default:
				result.AppendLine($"// Advanced Comment object action {actionNum} not implemented");
				break;
		}

		return result.ToString();
	}

	public override string ExportExpression(Expression expression, EventBase eventBase = null)
	{
		string result;

		switch (expression.Num)
		{
			default:
				result = $"(/* Advanced Comment object expression {expression.Num} not implemented */";
				break;
		}

		return result;
	}
}
