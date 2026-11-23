using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Expressions;
using System.Text;

public class WindowControlExporter : ExtensionExporter
{
	public override string ObjectIdentifier => "LTCW";
	public override string ExtensionName => "kcwctrl";
	public override string CppClassName => "WindowControlExtension";

	public override string ExportExtension(byte[] extensionData)
	{
		return CreateExtension("");
	}

	public override string ExportCondition(EventBase eventBase, int conditionNum, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (", bool isGlobal = false)
	{
		StringBuilder result = new StringBuilder();

		switch (conditionNum)
		{
			default:
				result.AppendLine($"// Window Control condition {conditionNum} not implemented");
				result.AppendLine($"goto {nextLabel};");
				break;
		}

		return result.ToString();
	}

	public override string ExportAction(EventBase eventBase, int actionNum, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, bool isGlobal = false)
	{
		StringBuilder result = new StringBuilder();

		switch (actionNum)
		{
			default:
				result.AppendLine($"// Window Control action {actionNum} not implemented");
				break;
		}

		return result.ToString();
	}

	public override string ExportExpression(Expression expression, EventBase eventBase = null)
	{
		string result;

		switch (expression.Num)
		{
			case 13: // Get Frame Win X Size
				result = $"{GetExtensionInstance(expression.ObjectInfo, expression.ObjectType)}->GetFrameWinXSize()";
				break;
			case 14: // Get Frame Win Y Size
				result = $"{GetExtensionInstance(expression.ObjectInfo, expression.ObjectType)}->GetFrameWinYSize()";
				break;
			default:
				result = $"(/* Window Control expression {expression.Num} not implemented */";
				break;
		}

		return result;
	}
}
