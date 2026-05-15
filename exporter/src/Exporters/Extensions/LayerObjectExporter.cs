using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Expressions;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class LayerObjectExporter : ExtensionExporter
{
	public override string ObjectIdentifier => "RYAL";
	public override string ExtensionName => "Layer";
	public override string CppClassName => "LayerObjectExtension";

	public override string ExportExtension(byte[] extensionData)
	{
		return CreateExtension("");
	}

	public override string ExportCondition(EventBase eventBase, int conditionNum, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (", bool isGlobal = false)
	{
		StringBuilder result = new();

		switch (conditionNum)
		{
			default:
				result.AppendLine($"// Layer Object condition {conditionNum} not implemented");
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
			case 12:
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->XSort(false);");
				break;
			case 13:
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->YSort(false);");
				break;
			case 14:
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->XSort();");
				break;
			case 15:
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->YSort();");
				break;
			case 26:
				{
					int altIndex = ((AlterableValue)eventBase.Items[0].Loader).Value;
					string defaultValue = ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase);
					result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->AltValueSort(false, {altIndex}, {defaultValue});");
				}
				break;
			case 27:
				{
					int altIndex = ((AlterableValue)eventBase.Items[0].Loader).Value;
					string defaultValue = ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase);
					result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->AltValueSort(true, {altIndex}, {defaultValue});");
				}
				break;
			default:
				result.AppendLine($"// Layer Object action {actionNum} not implemented");
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
				result = $"0 /* Layer Object expression {expression.Num} not implemented */";
				break;
		}

		return result;
	}
}