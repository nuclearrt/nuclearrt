using CTFAK.Memory;
using CTFAK.CCN.Chunks.Frame;
using System.Text;
using CTFAK.MMFParser.EXE.Loaders.Events.Expressions;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class IniExporter : ExtensionExporter
{
	public override string ObjectIdentifier => "0INI";
	public override string ExtensionName => "kcini";
	public override string CppClassName => "IniExtension";

	public override string ExportExtension(byte[] extensionData)
	{
		ByteReader reader = new ByteReader(extensionData);

		short flags = reader.ReadInt16();
		string name = reader.ReadWideString();
		if (string.IsNullOrWhiteSpace(name))
		{
			name = "default.ini";
		}

		return CreateExtension($"{flags}, \"{name}\"");
	}

	public override string ExportCondition(EventBase eventBase, int conditionNum, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (", bool isGlobal = false)
	{
		StringBuilder result = new();

		switch (conditionNum)
		{
			default:
				result.AppendLine($"// Ini condition {conditionNum} not implemented");
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
			case 0: // Set Current Group
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SetCurrentGroup({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			case 1: // Set Current Item
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SetCurrentItem({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			case 2: // Set Value
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SetValue({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			case 3: // Save Position
				ParamObject paramObject = (ParamObject)eventBase.Items[0].Loader;
				string objectSelector = ExpressionConverter.GetSelector(paramObject.ObjectInfo);
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SavePosition(&(**{objectSelector}->begin()));");
				break;
			case 4: // Load Position
				ParamObject paramObject2 = (ParamObject)eventBase.Items[0].Loader;
				string objectSelector2 = ExpressionConverter.GetSelector(paramObject2.ObjectInfo);
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->LoadPosition(&(**{objectSelector2}->begin()));");
				break;
			case 6: // Set File Name
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SetFileName({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			case 7: // Set Value (Item)
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SetValue({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)});");
				break;
			case 8: // Set Value (Group - Item)
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SetValue({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[2].Loader, eventBase)});");
				break;
			case 5: // Set String
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SetString({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			case 9: // Set String (Item)
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SetString({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)});");
				break;
			case 10: // Set String (Group - Item)
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SetString({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[2].Loader, eventBase)});");
				break;
			case 11: // Delete Item
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->DeleteItem({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			case 12: // Delete Item (Group - Item)
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->DeleteItem({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)});");
				break;
			case 13: // Delete Group
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->DeleteGroup({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			default:
				result.AppendLine($"// Ini action {actionNum} not implemented");
				break;
		}

		return result.ToString();
	}

	public override string ExportExpression(Expression expression, EventBase eventBase = null)
	{
		string result;

		switch (expression.Num)
		{
			case 0: // Get Value
			case 2: // Get Value (Item)
			case 3: // Get Value (Group - Item)
				result = $"{GetExtensionInstance(expression.ObjectInfo)}->GetValue(";
				break;
			case 1: // Get String
			case 4: // Get String (Item)
			case 5: // Get String (Group - Item)
				result = $"{GetExtensionInstance(expression.ObjectInfo)}->GetString(";
				break;
			default:
				result = $"0 /* Ini expression {expression.Num} not implemented */";
				break;
		}

		return result;
	}
}
