using System.Diagnostics.Contracts;
using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.Memory;
using CTFAK.MMFParser.EXE.Loaders.Events.Expressions;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;
using CTFAK.Utils;

public class GlobalStoreXExporter : ExtensionExporter
{
	public override string ObjectIdentifier => "RTSD";
	public override string ExtensionName => "gstorex";
	public override string CppClassName => "GlobalStoreXExtension";

	public override string ExportExtension(byte[] extensionData)
	{
		ByteReader reader = new ByteReader(extensionData);
		reader.Skip(8); // 8 bytes unknown

		uint IntegerCount = reader.ReadUInt32();
		uint StringCount = reader.ReadUInt32();
		uint BoolCount = reader.ReadUInt32();
		uint ShortCount = reader.ReadUInt32();

		int flags = 0;
		if (!reader.ReadBoolean()) flags |= 0x01; // integer 
		if (!reader.ReadBoolean()) flags |= 0x02; // string 
		if (!reader.ReadBoolean()) flags |= 0x04; // bool 
		if (!reader.ReadBoolean()) flags |= 0x08; // short 

		return CreateExtension($"{IntegerCount}, {StringCount}, {BoolCount}, {ShortCount}, {flags}");
	}

	public override string ExportCondition(EventBase eventBase, int conditionNum, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (", bool isGlobal = false)
	{
		StringBuilder result = new();

		switch (conditionNum)
		{
			case 1: // Compare Integer
				result.AppendLine($"{ifStatement} ({GetExtensionInstance(eventBase.ObjectInfo)}->GetInteger({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}) {ExpressionConverter.GetComparisonSymbol(((ExpressionParameter)eventBase.Items[1].Loader).Comparsion)} {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)})) goto {nextLabel};");
				break;
			case 2: // Compare Bool
				result.AppendLine($"{ifStatement} ({GetExtensionInstance(eventBase.ObjectInfo)}->GetBool({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}) {ExpressionConverter.GetComparisonSymbol(((ExpressionParameter)eventBase.Items[1].Loader).Comparsion)} {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)})) goto {nextLabel};");
				break;
			case 3: // Compare Short
				result.AppendLine($"{ifStatement} ({GetExtensionInstance(eventBase.ObjectInfo)}->GetShort({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}) {ExpressionConverter.GetComparisonSymbol(((ExpressionParameter)eventBase.Items[1].Loader).Comparsion)} {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)})) goto {nextLabel};");
				break;
			default:
				result.AppendLine($"// Global Store X condition {conditionNum} not implemented");
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
			case 0: // Set Integer
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SetInteger({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)});");
				break;
			case 1: // Set Bool
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SetBool({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)});");
				break;
			case 2: // Set String
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SetString({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)});");
				break;
			case 3: // Add Integer
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->AddInteger({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)});");
				break;
			case 4: // Set Bool Range
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SetRangeBool({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[2].Loader, eventBase)});");
				break;
			case 5: // Add String
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->AddString({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)});");
				break;
			case 6: // Subtract Integer
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SubtractInteger({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)});");
				break;
			case 7: // Clear Bools
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->ClearBools();");
				break;
			case 8: // Set Integer Range
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SetRangeInteger({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[2].Loader, eventBase)});");
				break;
			case 9: // Set String Range
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SetRangeString({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[2].Loader, eventBase)});");
				break;
			case 10: // Clear Integers
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->ClearIntegers();");
				break;
			case 11: // Resize Integers
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->ResizeIntegers({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			case 12: // Expand Integers
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->ExpandIntegers({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			case 13: // Clear Strings
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->ClearStrings();");
				break;
			case 14: // Resize Strings
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->ResizeStrings({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			case 15: // Expand Strings
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->ExpandStrings({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			case 16: // Resize Bools
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->ResizeBools({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			case 17: // Expand Bools
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->ExpandBools({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			case 18: // Save Integer INI
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SaveIntegerINI({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)});");
				break;
			case 19: // Save String INI
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SaveStringINI({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)});");
				break;
			case 20: // Save Bool INI
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SaveBoolINI({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)});");
				break;
			case 21: // Save All INI
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SaveAllINI({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)});");
				break;
			case 22: // Load Integer INI
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->LoadIntegerINI({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)});");
				break;
			case 23: // Load String INI
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->LoadStringINI({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)});");
				break;
			case 24: // Load Bool INI
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->LoadBoolINI({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)});");
				break;
			case 25: // Load All INI
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->LoadAllINI({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)});");
				break;
			case 26: // Save Integer Binary
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SaveIntegerBinary({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			case 27: // Save String Binary
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SaveStringBinary({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			case 28: // Save Bool Binary
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SaveBoolBinary({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			case 29: // Save All Binary
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SaveAllBinary({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			case 30: // Load Integer Binary
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->LoadIntegerBinary({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			case 31: // Load String Binary
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->LoadStringBinary({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			case 32: // Load Bool Binary
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->LoadBoolBinary({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			case 33: // Load All Binary
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->LoadAllBinary({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			case 34: // Set Short
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SetShort({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)});");
				break;
			case 35: // Add Short
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->AddShort({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)});");
				break;
			case 36: // Subtract Short
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SubtractShort({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)});");
				break;
			case 37: // Set Short Range
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SetRangeShort({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[2].Loader, eventBase)});");
				break;
			case 38: // Clear Shorts
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->ClearShorts();");
				break;
			case 39: // Resize Shorts
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->ResizeShorts({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			case 40: // Expand Shorts
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->ExpandShorts({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			case 41: // Save Short INI
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SaveShortINI({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			case 42: // Load Short INI
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->LoadShortINI({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			case 43: // Save Short Binary
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SaveShortBinary({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			case 44: // Load Short Binary
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->LoadShortBinary({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			default:
				result.AppendLine($"// Global Store X action {actionNum} not implemented");
				break;
		}

		return result.ToString();
	}

	public override string ExportExpression(Expression expression, EventBase eventBase = null)
	{
		string result;

		switch (expression.Num)
		{
			case 0: // Get Integer
				result = $"{GetExtensionInstance(expression.ObjectInfo)}->GetInteger(";
				break;
			case 1: // Get Bool
				result = $"{GetExtensionInstance(expression.ObjectInfo)}->GetBool(";
				break;
			case 2: // Get String
				result = $"{GetExtensionInstance(expression.ObjectInfo)}->GetString(";
				break;
			case 3: // Get Integer Array Size
				result = $"{GetExtensionInstance(expression.ObjectInfo)}->GetIntegerArraySize()";
				break;
			case 4: // Get String Array Size
				result = $"{GetExtensionInstance(expression.ObjectInfo)}->GetStringArraySize()";
				break;
			case 5: // Get Integer Base
				result = $"{GetExtensionInstance(expression.ObjectInfo)}->GetIntegerBase()";
				break;
			case 6: // Get String Base
				result = $"{GetExtensionInstance(expression.ObjectInfo)}->GetStringBase()";
				break;
			case 7: // Get Bool Size
				result = $"{GetExtensionInstance(expression.ObjectInfo)}->GetBoolArraySize()";
				break;
			case 8: // Get Bool Base
				result = $"{GetExtensionInstance(expression.ObjectInfo)}->GetBoolBase()";
				break;
			case 9: // Get Short
				result = $"{GetExtensionInstance(expression.ObjectInfo)}->GetShort(";
				break;
			case 10: // Get Short Size
				result = $"{GetExtensionInstance(expression.ObjectInfo)}->GetShortArraySize()";
				break;
			case 11: // Get Short Base
				result = $"{GetExtensionInstance(expression.ObjectInfo)}->GetShortBase()";
				break;
			default:
				result = $"0 /* Global Store X expression {expression.Num} not implemented */";
				break;
		}

		return result;
	}
}