using CTFAK.Memory;
using CTFAK.CCN.Chunks.Frame;
using System.Text;
using CTFAK.MMFParser.EXE.Loaders.Events.Expressions;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class KcRuntimeExporter : ExtensionExporter
{
	public override string ObjectIdentifier => "emTR"; // Or RTme
	public override string ExtensionName => "KcRuntime";
	public override string CppClassName => "KcRuntimeExtension";

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
				result.AppendLine($"{ifStatement} {GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}-IsRuntimeAnaconda()) goto {nextLabel};");
				break;
			case 1:
				result.AppendLine($"{ifStatement} {GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->IsRuntimeAndroid()) goto {nextLabel};");
				break;
			case 2:
				result.AppendLine($"{ifStatement} {GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->IsRuntimeExeStandard()) goto {nextLabel};");
				break;
			case 3:
				result.AppendLine($"{ifStatement} {GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->IsRuntimeExeHwa()) goto {nextLabel};");
				break;
			case 4:
				result.AppendLine($"{ifStatement} {GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->IsRuntimeiOS()) goto {nextLabel};");
				break;
			case 5:
				result.AppendLine($"{ifStatement} {GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->IsRuntimeJava()) goto {nextLabel};");
				break;
			case 6:
				result.AppendLine($"{ifStatement} {GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->IsRuntimeJavaMobile()) goto {nextLabel};");
				break;
			case 7:
				result.AppendLine($"{ifStatement} {GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->IsRuntimeMac()) goto {nextLabel};");
				break;
			case 8:
				result.AppendLine($"{ifStatement} {GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->IsRuntimeSwf()) goto {nextLabel};");
				break;
			case 9:
				result.AppendLine($"{ifStatement} {GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->IsRuntimeXna()) goto {nextLabel};");
				break;
			default:
				result.AppendLine($"// Runtime object condition {conditionNum} not implemented");
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
				result.AppendLine($"// Runtime object action {actionNum} not implemented");
				break;
		}

		return result.ToString();
	}

	public override string ExportExpression(Expression expression, EventBase eventBase = null)
	{
		string result;

		switch (expression.Num)
		{
			case 0: // Get Runtime Name
				result = $"{GetExtensionInstance(expression.ObjectInfo, expression.ObjectType)}->GetRuntimeName(";
				break;
			default:
				result = $"(/* Runtime object expression {expression.Num} not implemented */";
				break;
		}

		return result;
	}
}
