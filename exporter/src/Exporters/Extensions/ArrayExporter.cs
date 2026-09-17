using CTFAK.Memory;
using CTFAK.CCN.Chunks.Frame;
using System.Text;
using CTFAK.MMFParser.EXE.Loaders.Events.Expressions;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class ArrayExporter : ExtensionExporter
{
	public override string ObjectIdentifier => "0RRA";
	public override string ExtensionName => "KcArray";
	public override string CppClassName => "ArrayExtension";

	public override string ExportExtension(byte[] extensionData)
	{
		ByteReader reader = new ByteReader(extensionData);

		int dimensionX = reader.ReadInt32();
		int dimensionY = reader.ReadInt32();
		int dimensionZ = reader.ReadInt32();
		int flags = reader.ReadInt32();

		return CreateExtension($"{dimensionX}, {dimensionY}, {dimensionZ}, {flags}");
	}

	public override string ExportCondition(EventBase eventBase, int conditionNum, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (", bool isGlobal = false)
	{
		StringBuilder result = new();

		switch (conditionNum)
		{
			case 0: // Is the index to the X dimension at end?
				result.AppendLine($"for (ObjectIterator it({GetSelector(eventBase.ObjectInfo, eventBase.ObjectType)}); !it.end(); ++it) {{");
				result.AppendLine($"    auto instance = *it;");
				result.AppendLine($"    {ifStatement} {GetExtensionInstanceLoop()}->Data->IsXAtEnd()) it.deselect();");
				result.AppendLine("}");
				result.AppendLine($"if ({GetSelector(eventBase.ObjectInfo, eventBase.ObjectType)}.Count() == 0) goto {nextLabel};");
				break;
			case 1: // Is the index to the Y dimension at end?
				result.AppendLine($"for (ObjectIterator it({GetSelector(eventBase.ObjectInfo, eventBase.ObjectType)}); !it.end(); ++it) {{");
				result.AppendLine($"    auto instance = *it;");
				result.AppendLine($"    {ifStatement} {GetExtensionInstanceLoop()}->Data->IsYAtEnd()) it.deselect();");
				result.AppendLine("}");
				result.AppendLine($"if ({GetSelector(eventBase.ObjectInfo, eventBase.ObjectType)}.Count() == 0) goto {nextLabel};");
				break;
			case 2: // Is the index to the Z dimension at end?
				result.AppendLine($"for (ObjectIterator it({GetSelector(eventBase.ObjectInfo, eventBase.ObjectType)}); !it.end(); ++it) {{");
				result.AppendLine($"    auto instance = *it;");
				result.AppendLine($"    {ifStatement} {GetExtensionInstanceLoop()}->Data->IsZAtEnd()) it.deselect();");
				result.AppendLine("}");
				result.AppendLine($"if ({GetSelector(eventBase.ObjectInfo, eventBase.ObjectType)}.Count() == 0) goto {nextLabel};");
				break;
			default:
				result.AppendLine($"// Array condition {conditionNum} not implemented");
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
			case 0: // Set Index A
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->Data->SetIndexA({EvaluateExpression(eventBase, 0)});");
				break;
			case 1: // Set Index B
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->Data->SetIndexB({EvaluateExpression(eventBase, 0)});");
				break;
			case 2: // Set Index C
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->Data->SetIndexC({EvaluateExpression(eventBase, 0)});");
				break;
			case 3: // Add Index A
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->Data->AddIndexA();");
				break;
			case 4: // Add Index B
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->Data->AddIndexB();");
				break;
			case 5: // Add Index C
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->Data->AddIndexC();");
				break;
			case 6: // Write Value at Index
			case 7: // Write String at Index
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->Data->WriteValueAtIndex({EvaluateExpression(eventBase, 0)});");
				break;
			case 8: // Clear
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->Data->Clear();");
				break;
			case 13: // Write Value at X
			case 16: // Write String at X
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->Data->WriteX({EvaluateExpression(eventBase, 1)}, {EvaluateExpression(eventBase, 0)});");
				break;
			case 14: // Write Value at XY
			case 17: // Write String at XY
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->Data->WriteXY({EvaluateExpression(eventBase, 1)}, {EvaluateExpression(eventBase, 2)}, {EvaluateExpression(eventBase, 0)});");
				break;
			case 15: // Write Value at XYZ
			case 18: // Write String at XYZ
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo, eventBase.ObjectType)}->Data->WriteXYZ({EvaluateExpression(eventBase, 1)}, {EvaluateExpression(eventBase, 2)}, {EvaluateExpression(eventBase, 3)}, {EvaluateExpression(eventBase, 0)});");
				break;
			default:
				result.AppendLine($"// Array action {actionNum} not implemented");
				break;
		}

		return result.ToString();
	}

	public override string ExportExpression(Expression expression, EventBase eventBase = null)
	{
		string result;

		switch (expression.Num)
		{
			case 0: // X Index
				result = $"{GetExtensionInstance(expression.ObjectInfo, expression.ObjectType)}->Data->GetXIndex()";
				break;
			case 1: // Y Index
				result = $"{GetExtensionInstance(expression.ObjectInfo, expression.ObjectType)}->Data->GetYIndex()";
				break;
			case 2: // Z Index
				result = $"{GetExtensionInstance(expression.ObjectInfo, expression.ObjectType)}->Data->GetZIndex()";
				break;
			case 3: // Value at Index
			case 4: // String at Index
				result = $"{GetExtensionInstance(expression.ObjectInfo, expression.ObjectType)}->Data->GetValueAtIndex()";
				break;
			case 5: // Value at X
			case 8: // String at X
				result = $"{GetExtensionInstance(expression.ObjectInfo, expression.ObjectType)}->Data->GetValueAtX(";
				break;
			case 6: // Value at XY
			case 9: // String at XY
				result = $"{GetExtensionInstance(expression.ObjectInfo, expression.ObjectType)}->Data->GetValueAtXY(";
				break;
			case 7: // Value at XYZ
			case 10: // String at XYZ
				result = $"{GetExtensionInstance(expression.ObjectInfo, expression.ObjectType)}->Data->GetValueAtXYZ(";
				break;
			case 11: // X Dimension
				result = $"{GetExtensionInstance(expression.ObjectInfo, expression.ObjectType)}->Data->GetXDimension()";
				break;
			case 12: // Y Dimension
				result = $"{GetExtensionInstance(expression.ObjectInfo, expression.ObjectType)}->Data->GetYDimension()";
				break;
			case 13: // Z Dimension
				result = $"{GetExtensionInstance(expression.ObjectInfo, expression.ObjectType)}->Data->GetZDimension()";
				break;
			default:
				result = $"0 /* Array expression {expression.Num} not implemented */";
				break;
		}

		return result;
	}
}
