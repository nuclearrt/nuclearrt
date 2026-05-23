using CTFAK.Memory;
using CTFAK.CCN.Chunks.Frame;
using System.Text;
using CTFAK.MMFParser.EXE.Loaders.Events.Expressions;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class IniPlusPlusExporter : ExtensionExporter
{
	public override string ObjectIdentifier => "\u00A0\u0053\u00FD\u0012";
	public override string ExtensionName => "INI++15";
	public override string CppClassName => "IniPlusPlusExtension";

	public override string ExportExtension(byte[] extensionData)
	{
		ByteReader reader = new ByteReader(extensionData);

		string DefaultFile = reader.ReadBoolean().ToString().ToLower();
		string ReadOnly = reader.ReadBoolean().ToString().ToLower();
		string DefaultFilePath = reader.ReadAscii(259).Split('\0')[0];
		reader.Skip(3);
		byte BaseFolder = reader.ReadByte();
		reader.Skip(3);
		string InitialData = reader.ReadAscii(2999).TrimEnd('\0');
		reader.Skip(1);
		string CreateFolders = reader.ReadBoolean().ToString().ToLower();
		string EnableAutoSave = reader.ReadBoolean().ToString().ToLower();
		string UseStandardSettings = reader.ReadBoolean().ToString().ToLower();
		string UseCompression = reader.ReadBoolean().ToString().ToLower();
		string UseEncryption = reader.ReadBoolean().ToString().ToLower();
		string EncryptionKey = reader.ReadAscii(31).TrimEnd('\0');
		reader.Skip(97);
		string NewLine = reader.ReadBoolean().ToString().ToLower();
		string NewLineText = reader.ReadAscii(9).TrimEnd('\0');
		reader.Skip(1);
		string AlwaysQuoteStrings = reader.ReadBoolean().ToString().ToLower();
		reader.Skip(3);
		byte RepeatedGroups = reader.ReadByte();
		reader.Skip(3);
		byte RepeatedItems = reader.ReadByte();
		byte UndoBufferSize = reader.ReadByte();
		byte RedoBufferSize = reader.ReadByte();
		reader.Skip(1);
		string SaveRepeatedItems = reader.ReadBoolean().ToString().ToLower();
		string EscapeCharsInGroupNames = reader.ReadBoolean().ToString().ToLower();
		string EscapeCharsInItemNames = reader.ReadBoolean().ToString().ToLower();
		string EscapeCharsInItemValues = reader.ReadBoolean().ToString().ToLower();
		string CaseSensitive = reader.ReadBoolean().ToString().ToLower();
		string GlobalData = reader.ReadBoolean().ToString().ToLower();
		string Index1Based = reader.ReadBoolean().ToString().ToLower();
		string EnableAutoLoad = reader.ReadBoolean().ToString().ToLower();
		string LoadAndSaveSubGroups = reader.ReadBoolean().ToString().ToLower();
		string AllowEmptyGroups = reader.ReadBoolean().ToString().ToLower();
		string GlobalDataKey = reader.ReadAscii(31).TrimEnd('\0');

		return CreateExtension($"{DefaultFile}, {ReadOnly}, \"{DefaultFilePath}\", {BaseFolder}, \"{InitialData}\", {CreateFolders}, {EnableAutoSave}, {UseStandardSettings}, {UseCompression}, {UseEncryption}, \"{EncryptionKey}\", {NewLine}, \"{NewLineText}\", {AlwaysQuoteStrings}, {RepeatedGroups}, {RepeatedItems}, {UndoBufferSize}, {RedoBufferSize}, {SaveRepeatedItems}, {EscapeCharsInGroupNames}, {EscapeCharsInItemNames}, {EscapeCharsInItemValues}, {CaseSensitive}, {GlobalData}, {Index1Based}, {EnableAutoLoad}, {LoadAndSaveSubGroups}, {AllowEmptyGroups}, \"{GlobalDataKey}\"");
	}

	public override string ExportCondition(EventBase eventBase, int conditionNum, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (", bool isGlobal = false)
	{
		StringBuilder result = new();

		switch (conditionNum)
		{
			case 0:
				result.AppendLine($"{ifStatement} {GetExtensionInstance(eventBase.ObjectInfo)}->CurrentGroupExists()) goto {nextLabel};");
				break;
			case 1:
				result.AppendLine($"{ifStatement} {GetExtensionInstance(eventBase.ObjectInfo)}->CurrentGroupItemExists({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)})) goto {nextLabel};");
				break;
			// case 2:
			//     return CurrentGroupCompareHashedItem(cnd);
			case 3:
				result.AppendLine($"{ifStatement} {GetExtensionInstance(eventBase.ObjectInfo)}->GroupItemExists({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)})) goto {nextLabel};");
				break;
			case 4:
				result.AppendLine($"{ifStatement} {GetExtensionInstance(eventBase.ObjectInfo)}->GroupExists({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)})) goto {nextLabel};");
				break;
			// case 5:
			//     return GroupCompareHashedItem(cnd);
			// case 18:
			//     return HasUndo();
			// case 19:
			//     return HasRedo();
			default:
				result.AppendLine($"// Ini++ condition {conditionNum} not implemented");
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
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SetCurrentGroup({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			case 1:
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SetCurrentGroupItemValue({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[2].Loader, eventBase)});");
				break;
			case 2:
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SetCurrentGroupItemString({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)});");
				break;
			// case 3:
			// 	SetCurrentGroupItemStringHashed(act);
			// 	break;
			// case 4:
			// 	SaveObjectInCurrentGroup(act);
			// 	break;
			// case 5:
			// 	LoadObjectInCurrentGroup(act);
			// 	break;
			// case 6:
			// 	SavePositionInCurrentGroup(act);
			// 	break;
			// case 7:
			// 	SaveGlobalValsInCurrentGroup();
			// 	break;
			// case 8:
			// 	LoadGlobalValsInCurrentGroup();
			// 	break;
			case 9:
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->DeleteCurrentGroup();");
				break;
			case 10:
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->DeleteCurrentGroupItem({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			// case 11:
			// 	RenameCurrentGroup(act);
			// 	break;
			// case 12:
			// 	RenameCurrentGroupItem(act);
			// 	break;
			// case 13:
			// 	MoveItemFromCurrentGroup(act);
			// 	break;
			case 14:
			 	result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SetItemValue({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[2].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[3].Loader, eventBase)});");
			 	break;
			case 15:
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SetItemString({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[2].Loader, eventBase)});");
			 	break;
			// case 16:
			// 	SetItemStringHashed(act);
			// 	break;
			// case 17:
			// 	SaveObject(act);
			// 	break;
			// case 18:
			// 	LoadObject(act);
			// 	break;
			// case 19:
			// 	SavePosition(act);
			// 	break;
			// case 20:
			// 	SaveGlobalVals(act);
			// 	break;
			// case 21:
			// 	LoadGlobalVals(act);
			// 	break;
			// case 22:
			// 	RenameGroup(act);
			// 	break;
			// case 23:
			// 	RenameGroupItem(act);
			// 	break;
			// case 24:
			// 	MoveItem(act);
			// 	break;
			// case 25:
			// 	MoveItemFromGroup(act);
			// 	break;
			// case 26:
			// 	CopyGroup(act);
			// 	break;
			// case 27:
			// 	CopyItem(act);
			// 	break;
			case 28:
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->DeleteGroup({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
				break;
			case 29:
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->DeleteGroupItem({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)});");
				break;
			// case 30:
			// 	DeleteMassItem(act);
			// 	break;
			// case 31:
			// 	DeleteWithPatterns(act);
			// 	break;
			case 32:
			 	result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->ClearINI();");
			 	break;
			case 44:
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->Save();");
				break;
			// case 79:
			// 	Undo();
			// 	break;
			// case 80:
			// 	Redo();
			// 	break;
			default:
				result.AppendLine($"// Ini++ action {actionNum} not implemented");
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
				result = $"{GetExtensionInstance(expression.ObjectInfo)}->GetCurrentGroupValue(";
				break;
			case 1:
				result = $"{GetExtensionInstance(expression.ObjectInfo)}->GetCurrentGroupString(";
				break;
			// case 2:
			// 	return Exp_GetCurrentGroupPositionX_Execute();
			// case 3:
			// 	return Exp_GetCurrentGroupPositionY_Execute();
			// case 4:
			// 	return Exp_GetCurrentGroupItemNameFromID_Execute();
			// case 5:
			// 	return Exp_GetCurrentGroupValueFromID_Execute();
			// case 6:
			// 	return Exp_GetCurrentGroupStringFromID_Execute();
			// case 7:
			// 	return Exp_GetCurrentGroupItemCount_Execute();
			case 8:
				result = $"{GetExtensionInstance(expression.ObjectInfo)}->GetItemValue(";
				break;
			case 9:
				result = $"{GetExtensionInstance(expression.ObjectInfo)}->GetItemString(";
				break;
			// case 10:
			// 	return Exp_GetItemPositionX_Execute();
			// case 11:
			// 	return Exp_GetItemPositionY_Execute();
			// case 12:
			// 	return Exp_GetGroupFromID_Execute();
			// case 13:
			// 	return Exp_GetGroupItemNameFromID_Execute();
			// case 14:
			// 	return Exp_GetGroupStringFromID_Execute();
			// case 15:
			// 	return Exp_GetGroupValueFromID_Execute();
			// case 16:
			// 	return Exp_GetGroupCount_Execute();
			// case 17:
			// 	return Exp_GetGroupItemCount_Execute();
			// case 18:
			// 	return Exp_GetTotalItemCount_Execute();
			// case 25:
			// 	return Exp_GetIniAsString_Execute();
			// case 26:
			// 	return Exp_GetHashedString_Execute();
			// case 28:
			// 	return Exp_GetEscapedString_Execute();
			// case 29:
			// 	return Exp_GetUnescapedString_Execute();
			// case 34:
			// 	return Exp_GetIniAsCSV_Execute();
			// case 35:
			// 	return Exp_GetItemNameFromID_Execute();
			// case 36:
			// 	return Exp_GetUniqueItemCount_Execute();
			// case 37:
			// 	return Exp_GetItemPart_Execute();
			// case 38:
			// 	return Exp_GetCurrentGroup_Execute();
			// case 39:
			// 	return Exp_GetCurrentGroupAsString_Execute();
			// case 40:
			// 	return Exp_GetCurrentFilePath_Execute();
			default:
				result = $"0 /* Ini++ expression {expression.Num} not implemented */";
				break;
		}

		return result;
	}
}
