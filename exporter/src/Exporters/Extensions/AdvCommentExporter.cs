using CTFAK.Memory;
using CTFAK.CCN.Chunks.Frame;
using System.Text;
using CTFAK.MMFParser.EXE.Loaders.Events.Expressions;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class AdvCommentExporter : ExtensionExporter
{
	public override string ObjectIdentifier => "FAOC";
	public override string ExtensionName => "AdvComment";
	public override string CppClassName => "AdvCommentExtension";

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
			case 1:
				result.AppendLine($"{ifStatement} {GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->CndCommentObject()) goto {nextLabel};");
				break;
			case 2:
				result.AppendLine($"{ifStatement} {GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->CndTextRegComment()) goto {nextLabel};");
				break;
			case 3:
				result.AppendLine($"{ifStatement} {GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->CndTextRegNote()) goto {nextLabel};");
				break;
			case 4:
				result.AppendLine($"{ifStatement} {GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->CndTextRegReminder()) goto {nextLabel};");
				break;
			case 5:
				result.AppendLine($"{ifStatement} {GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->CndTextRegImportant()) goto {nextLabel};");
				break;
			case 6:
				result.AppendLine($"{ifStatement} {GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->CndTextRegAAE()) goto {nextLabel};");
				break;
			case 7:
				result.AppendLine($"{ifStatement} {GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->CndTextCapComment()) goto {nextLabel};");
				break;
			case 8:
				result.AppendLine($"{ifStatement} {GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->CndTextCapNote()) goto {nextLabel};");
				break;
			case 9:
				result.AppendLine($"{ifStatement} {GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->CndTextCapReminder()) goto {nextLabel};");
				break;
			case 10:
				result.AppendLine($"{ifStatement} {GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->CndTextCapImportant()) goto {nextLabel};");
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
			case 1:
			 	result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->ActTextRegComment();");
			 	break;
			case 2:
			 	result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->ActTextRegNote();");
			 	break;
			case 3:
			 	result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->ActTextRegReminder();");
			 	break;
			case 4:
			 	result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->ActTextRegImportant();");
			 	break;
			case 5:
			 	result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->ActTextRegAAA();");
			 	break;
			case 6:
			 	result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->ActTextRegAAB();");
			 	break;
			case 7:
			 	result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->ActTextCapComment();");
			 	break;
			case 8:
			 	result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->ActTextCapNote();");
			 	break;
			case 9:
			 	result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->ActTextCapReminder();");
			 	break;
			case 10:
			 	result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->ActTextCapImportant();");
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
