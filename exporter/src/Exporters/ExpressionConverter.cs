using Avalonia.Styling;
using CTFAK.CCN;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.CCN.Chunks.Objects;
using CTFAK.MFA;
using CTFAK.MMFParser.EXE.Loaders.Events.Expressions;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;
using CTFAK.Utils;
using System.Text;
using static CTFAK.CCN.Constants;

// TODO: use enums for each objetc expression id for readablity?
public class ExpressionConverter
{
	private readonly Exporter _exporter;

	public ExpressionConverter(Exporter exporter)
	{
		_exporter = exporter;
	}

	private enum ObjectType
	{
		Player = -7,
		Keyboard = -6,
		Create = -5,
		Timer = -4,
		Game = -3,
		Speaker = -2,
		System = -1,
		Arithmetic = 0,
		Backdrop = 1,
		Active = 2,
		Text = 3,
		Question = 4,
		Score = 5,
		Lives = 6,
		Counter = 7,
		Rtf = 8,
		SubApplication = 9,
		Extension = 32,
	}

	private static readonly Dictionary<(ObjectType, int), Func<Expression, string>> expressionsLookup = new()
	{
        //Player
        { (ObjectType.Player, 1), e => $"Application::Instance().GetAppData()->GetPlayerLives({e.ObjectInfo})" }, // Player Lives
		{ (ObjectType.Player, 2), e => $"Application::Instance().GetInput()->GetControlType({e.ObjectInfo})" }, // Player Input device

        //Keyboard / Mouse
        { (ObjectType.Keyboard, 0), _ => "GetMouseX()" }, // XMouse
        { (ObjectType.Keyboard, 1), _ => "GetMouseY()" }, // YMouse
        { (ObjectType.Keyboard, 2), _ => "Application::Instance().GetInput()->GetMouseWheelMove()" }, // WheelDelta

        //Create
        { (ObjectType.Create, 0), _ => $"ObjectInstances.size()" }, // Total Objects

        //Timer
        { (ObjectType.Timer, 0), _ => "GameTimer.GetTime()" }, // Timer
        { (ObjectType.Timer, 1), _ => "GameTimer.GetHundreds()" }, // Hundreds
        { (ObjectType.Timer, 2), _ => "GameTimer.GetSeconds()" }, // seconds
        { (ObjectType.Timer, 3), _ => "GameTimer.GetMinutes()" }, // Minutes
        { (ObjectType.Timer, 4), _ => "GameTimer.GetHours()" }, // Hours
        { (ObjectType.Timer, 5), _ => $"0" }, // Event Index // TODO

        //Game
        { (ObjectType.Game, 0),  _ => $"Index + 1" }, // Frame
		{ (ObjectType.Game, 2), _ => $"GetXLeftEdge()" }, // XLeftEdge
		{ (ObjectType.Game, 3), _ => $"GetXRightEdge()" }, // XRightEdge
		{ (ObjectType.Game, 4), _ => $"GetYTopEdge()" }, // YTopEdge
		{ (ObjectType.Game, 5), _ => $"GetYBottomEdge()" }, // YBottomEdge
		{ (ObjectType.Game, 6), _ => $"Width" }, // Frame Width
		{ (ObjectType.Game, 7), _ => $"Height" }, // Frame Height
        { (ObjectType.Game, 8),  _ => $"Index + 1" }, // Frame
        { (ObjectType.Game, 10), _ => "Application::Instance().GetAppData()->GetTargetFPS()" }, // FrameRate // TODO: Verify this
        { (ObjectType.Game, 14), _ => "0" }, // DisplayMode // TODO
        { (ObjectType.Game, 15), _ => "0" }, // PixelShaderVersion // TODO

		//Speaker
		{ (ObjectType.Speaker, 0), _ => "Application::Instance().GetBackend()->audio->GetSampleVolume(-1)" }, // Main Volume
		{ (ObjectType.Speaker, 1), e => $"Application::Instance().GetBackend()->audio->GetSampleVolume(" }, // Sample Volume
		{ (ObjectType.Speaker, 2), e => $"Application::Instance().GetBackend()->audio->GetChannelVolume("}, // Channel Volume
		{ (ObjectType.Speaker, 3), _ => "Application::Instance().GetBackend()->audio->GetSamplePan(-1, false)" }, // Main Pan
		{ (ObjectType.Speaker, 4), e => $"Application::Instance().GetBackend()->audio->GetSamplePan(Application::Instance().GetBackend()->audio->FindSample({(e.Loader as StringExp).Value}), false)" }, // Sample Pan
		{ (ObjectType.Speaker, 5), e => $"Application::Instance().GetBackend()->audio->GetSamplePan({(e.Loader as DoubleExp).Value}, true)"}, // Channel Pan
		{ (ObjectType.Speaker, 6), e => $"Application::Instance().GetBackend()->audio->GetSamplePos({(e.Loader as StringExp).Value}, false)" }, // Sample Position
		{ (ObjectType.Speaker, 7), e => $"Application::Instance().GetBackend()->audio->GetSamplePos({(e.Loader as DoubleExp).Value}, true)" }, // Channel Position
		{ (ObjectType.Speaker, 8), e => $"Application::Instance().GetBackend()->audio->GetSampleDuration({(e.Loader as StringExp).Value}, false)"}, // Sample Duration
		{ (ObjectType.Speaker, 9), e => $"Application::Instance().GetBackend()->audio->GetSampleDuration({(e.Loader as DoubleExp).Value}, true)" }, // Channel Duration
		{ (ObjectType.Speaker, 10), e => $"Application::Instance().GetBackend()->audio->GetSampleFreq({(e.Loader as StringExp).Value}, true" }, // Sample Frequency
		{ (ObjectType.Speaker, 11), e => $"Application::Instance().GetBackend()->audio->GetSampleFreq({(e.Loader as DoubleExp).Value}, true" }, // Channel Frequency
		{ (ObjectType.Speaker, 12), _ => $"Application::Instance().GetBackend()->audio->GetChannelName(" }, // Channel Sample Name

        // System
        { (ObjectType.System, -3), _ => ", " },
		{ (ObjectType.System, -2), _ => ")" },
		{ (ObjectType.System, -1), _ => "(" },
		{ (ObjectType.System, 1),  _ => "Application::Instance().Random(" }, // Random(
        { (ObjectType.System, 2),  _ => $"Application::Instance().GetAppData()->GetGlobalValue(" }, // Global Value
        { (ObjectType.System, 3),  e => $"std::string(\"{e.Loader.ToString()}\")" },
		{ (ObjectType.System, 4),  _ => $"std::to_string(" }, // Str$
		{ (ObjectType.System, 5),  _ => $"MathHelper::Stoi(" }, // Val(
        { (ObjectType.System, 6),  _ => "std::string(\"\")" }, // Appdrive$ // TODO
        { (ObjectType.System, 7),  _ => "std::string(\"\")" }, // Appdir$ // TODO
        { (ObjectType.System, 8),  _ => "std::string(\"\")" }, // Apppath$ // TODO
        { (ObjectType.System, 9),  _ => "std::string(\"\")" }, // Appname$ // TODO
		{ (ObjectType.System, 10), _ => "std::sin(" }, // Sin
		{ (ObjectType.System, 11), _ => "std::cos(" }, // Cos
		{ (ObjectType.System, 13),  _ => "std::sqrt(" }, // Square Root
        { (ObjectType.System, 19), _ => "StringLeft(" }, // String Left
        { (ObjectType.System, 20), _ => "StringRight(" }, // String Right
        { (ObjectType.System, 22), _ => "StringLength(" }, // String Length
		{ (ObjectType.System, 23), e => (e.Loader as DoubleExp).FloatValue.ToString() },
		{ (ObjectType.System, 24), e => $"Application::Instance().GetAppData()->GetGlobalValue({GetGlobalValueIndex(e.Loader as GlobalCommon)})" }, // Global Value
		{ (ObjectType.System, 29), _ => "std::abs(" }, // Abs(
		{ (ObjectType.System, 31), _ => "std::floor(" }, // Floor
		{ (ObjectType.System, 40), _ => "std::min(" }, // Min(
        { (ObjectType.System, 41), _ => "std::max(" }, // Max(
		{ (ObjectType.System, 42), _ => "GetRGB(" }, // GetRGB
        { (ObjectType.System, 46), _ => "Loopindex(" }, // LoopIndex
		{ (ObjectType.System, 48), _ => "std::round(" }, // Round
		{ (ObjectType.System, 50), e => $"Application::Instance().GetAppData()->GetGlobalString({GetGlobalValueIndex(e.Loader as GlobalCommon)})" },
		{ (ObjectType.System, 56), _ => "std::string(\"\")" }, // AppTempPath$ // TODO
        { (ObjectType.System, 65), _ => "Application::Instance().RandomRange(" }, // RRandom
        { (ObjectType.System, 67), _ => "Application::Instance().GetBackend()->platform->GetPlatformName()" }, // RuntimeName$

        // Arithmetic
        { (ObjectType.Arithmetic, 2), _ => " + " }, // Add
        { (ObjectType.Arithmetic, 4), _ => " - " }, // Sub
        { (ObjectType.Arithmetic, 6), _ => " * " }, // Multiply
        { (ObjectType.Arithmetic, 8), _ => " /MathHelper::GetSafeDivision()/ " }, // Division
		{ (ObjectType.Arithmetic, 10), _ => " % " }, // Mod
		{ (ObjectType.Arithmetic, 12), _ => " ^ " }, // Power

    };

	public static int GetGlobalValueIndex(GlobalCommon value)
	{
		if ((int)value.Value < short.MinValue)
		{
			return (int)value.Value + ushort.MaxValue + 1;
		}

		return (int)value.Value;
	}

	private static void HandleSystemExpr(StringBuilder stringBuilder, Expression expression, EventBase eventBase = null)
	{
		switch (expression.Num)
		{
			case 0: //
				ExpressionLoader loader = (ExpressionLoader)expression.Loader;

				if (loader is StringExp) stringBuilder.Append($"std::string(\"{(loader as StringExp).Value}\")");
				else if (loader is DoubleExp) stringBuilder.Append((loader as DoubleExp).FloatValue);
				else stringBuilder.Append(loader.Value.ToString());
				break;
			default:
				HandleUnimplemented(stringBuilder, expression, eventBase);
				break;
		}
	}
	private static StringBuilder HandleRuntimeObjectExpr(StringBuilder stringBuilder, Expression expression, EventBase eventBase = null)
	{
		// common expressions
		switch (expression.Num)
		{
			case 12: // Fixed Value
				return stringBuilder.Append("0"); // TODO
			case 15: // Number of this Object
				return stringBuilder.Append($"{GetSelector(expression.ObjectInfo)}->Size()");
			case 45: // Number of selected Objects
				return stringBuilder.Append($"{GetSelector(expression.ObjectInfo)}->Count()");
			case 46: // Instance Value
				return stringBuilder.Append("instance->InstanceValue");
			case 1: // Y Position
				{
					if (expression.ObjectInfo == eventBase.ObjectInfo && expression.ObjectInfoList == eventBase.ObjectInfoList)
						return stringBuilder.Append("instance->Y");
					else
						return stringBuilder.Append($"({GetSelector(expression.ObjectInfo)}->Count() > 0 ? (*{GetSelector(expression.ObjectInfo)}->begin())->Y : 0)");
				}
			case 11: // X Position
				{
					if (expression.ObjectInfo == eventBase.ObjectInfo && expression.ObjectInfoList == eventBase.ObjectInfoList)
						return stringBuilder.Append("instance->X");
					else
						return stringBuilder.Append($"({GetSelector(expression.ObjectInfo)}->Count() > 0 ? (*{GetSelector(expression.ObjectInfo)}->begin())->X : 0)");
				}
			case 13: // Alterable Flag
				{
					if (expression.ObjectInfo == eventBase.ObjectInfo && expression.ObjectInfoList == eventBase.ObjectInfoList)
						return stringBuilder.Append($"GetAlterableFlagValue(instance, ");
					else
						return stringBuilder.Append($"GetAlterableFlagValue({GetSelector(expression.ObjectInfo)}, ");
				}
			case 16: // Alterable Value
				{
					if (expression.ObjectInfo == eventBase.ObjectInfo && expression.ObjectInfoList == eventBase.ObjectInfoList)
						return stringBuilder.Append($"(({GetObjectClassName(expression.ObjectInfo)}*)instance)->Values.GetValue({((ShortExp)expression.Loader).Value})");
					else
						return stringBuilder.Append($"({GetSelector(expression.ObjectInfo)}->Count() > 0 ? (({GetObjectClassName(expression.ObjectInfo)}*)*({GetSelector(expression.ObjectInfo)}->begin()))->Values.GetValue({((ShortExp)expression.Loader).Value}) : 0)");
				}
			case 30: //Alterable Value by Index
				{
					if (expression.ObjectInfo == eventBase.ObjectInfo && expression.ObjectInfoList == eventBase.ObjectInfoList)
						return stringBuilder.Append($"GetAlterableValueByIndex(instance, ");
					else
						return stringBuilder.Append($"GetAlterableValueByIndex({GetSelector(expression.ObjectInfo)}, ");
				}
		}


		// object expressions
		// Extension
		if (expression.ObjectType >= 32)
		{
			var exporter = ExtensionExporterRegistry.GetExporterByObjectInfo(expression.ObjectInfo, Exporter.Instance.CurrentFrame);
			expression.Num -= 80;

			if (exporter == null)
			{
				ObjectCommon? common = Exporter.Instance.GameData.frameitems.GetValueOrDefault(GetObject(expression.ObjectInfo, false, Exporter.Instance.CurrentFrame).Item1)?.properties as ObjectCommon;
				Logger.Log($"Extension exporter not found for ObjectInfo {expression.ObjectInfo} ({common?.Identifier ?? ""})");
				stringBuilder.Append($"Extension exporter not found for ObjectInfo {expression.ObjectInfo} ({common?.Identifier ?? ""}). ({expression.ObjectType}, {expression.Num})");
				HandleUnimplemented(stringBuilder, expression, eventBase);
				return stringBuilder;
			}
			return stringBuilder.Append(exporter.ExportExpression(expression, eventBase));
		}

		// Active/Backdrop
		switch (expression.Num)
		{

			case 2: // Image
				{
					if (expression.ObjectInfo == eventBase.ObjectInfo && expression.ObjectInfoList == eventBase.ObjectInfoList)
						return stringBuilder.Append("((Active*)instance)->animations.GetCurrentFrameIndex()");
					else
						return stringBuilder.Append($"({GetSelector(expression.ObjectInfo)}->Count() > 0 ? ((Active*)*({GetSelector(expression.ObjectInfo)}->begin()))->animations.GetCurrentFrameIndex() : 0)");
				}
			case 3: // Real Movement Speed
				{
					if (expression.ObjectInfo == eventBase.ObjectInfo && expression.ObjectInfoList == eventBase.ObjectInfoList)
						return stringBuilder.Append($"(({GetObjectClassName(expression.ObjectInfo)}*)instance)->movements.GetCurrentMovement()->GetRealSpeed()");
					else
						return stringBuilder.Append($"({GetSelector(expression.ObjectInfo)}->Count() > 0 ? (({GetObjectClassName(expression.ObjectInfo)}*)*({GetSelector(expression.ObjectInfo)}->begin()))->movements.GetCurrentMovement()->GetRealSpeed() : 0)");
				}
			case 6: // Animation Direction
				{
					if (expression.ObjectInfo == eventBase.ObjectInfo && expression.ObjectInfoList == eventBase.ObjectInfoList)
						return stringBuilder.Append("((Active*)instance)->animations.GetCurrentDirection()");
					else
						return stringBuilder.Append($"({GetSelector(expression.ObjectInfo)}->Count() > 0 ? ((Active*)*({GetSelector(expression.ObjectInfo)}->begin()))->animations.GetCurrentDirection() : 0)");
				}
			case 14: // Animation Number
				{
					if (expression.ObjectInfo == eventBase.ObjectInfo && expression.ObjectInfoList == eventBase.ObjectInfoList)
						return stringBuilder.Append("((Active*)instance)->animations.GetCurrentSequenceIndex()");
					else
						return stringBuilder.Append($"({GetSelector(expression.ObjectInfo)}->Count() > 0 ? ((Active*)*({GetSelector(expression.ObjectInfo)}->begin()))->animations.GetCurrentSequenceIndex() : 0)");
				}
			case 25: // Action Point X
				{
					if (expression.ObjectInfo == eventBase.ObjectInfo && expression.ObjectInfoList == eventBase.ObjectInfoList)
						return stringBuilder.Append("((Active*)instance)->GetXActionPoint()");
					else
						return stringBuilder.Append($"({GetSelector(expression.ObjectInfo)}->Count() > 0 ? ((Active*)*({GetSelector(expression.ObjectInfo)}->begin()))->GetXActionPoint() : 0)");
				}
			case 26: // Action Point Y
				{
					if (expression.ObjectInfo == eventBase.ObjectInfo && expression.ObjectInfoList == eventBase.ObjectInfoList)
						return stringBuilder.Append("((Active*)instance)->GetYActionPoint()");
					else
						return stringBuilder.Append($"({GetSelector(expression.ObjectInfo)}->Count() > 0 ? ((Active*)*({GetSelector(expression.ObjectInfo)}->begin()))->GetYActionPoint() : 0)");
				}
			case 27: // Alpha Coefficient
				{
					if (expression.ObjectInfo == eventBase.ObjectInfo && expression.ObjectInfoList == eventBase.ObjectInfoList)
						return stringBuilder.Append("instance->GetEffectParameter()");
					else
						return stringBuilder.Append($"({GetSelector(expression.ObjectInfo)}->Count() > 0 ? (*{GetSelector(expression.ObjectInfo)}->begin())->GetEffectParameter() : 0)");
				}
			case 32: // Distance with a point
				{
					if (expression.ObjectInfo == eventBase.ObjectInfo && expression.ObjectInfoList == eventBase.ObjectInfoList)
						return stringBuilder.Append($"ODistance(instance, ");
					else
						return stringBuilder.Append($"ODistance({GetSelector(expression.ObjectInfo)}, ");
				}
			case 33: // Angle of a Vector
				{
					if (expression.ObjectInfo == eventBase.ObjectInfo && expression.ObjectInfoList == eventBase.ObjectInfoList)
						return stringBuilder.Append($"OAngle(instance, ");
					else
						return stringBuilder.Append($"OAngle({GetSelector(expression.ObjectInfo)}, ");
				}
			case 40: // Object Width
				{
					if (expression.ObjectInfo == eventBase.ObjectInfo && expression.ObjectInfoList == eventBase.ObjectInfoList)
						return stringBuilder.Append("((Active*)instance)->animations.GetWidth()");
					else
						return stringBuilder.Append($"({GetSelector(expression.ObjectInfo)}->Count() > 0 ? ((Active*)*({GetSelector(expression.ObjectInfo)}->begin()))->animations.GetWidth() : 0)");
				}
			case 41: // Object Height
				{
					if (expression.ObjectInfo == eventBase.ObjectInfo && expression.ObjectInfoList == eventBase.ObjectInfoList)
						return stringBuilder.Append("((Active*)instance)->animations.GetHeight()");
					else
						return stringBuilder.Append($"({GetSelector(expression.ObjectInfo)}->Count() > 0 ? ((Active*)*({GetSelector(expression.ObjectInfo)}->begin()))->animations.GetHeight() : 0)");
				}
		}

		if (expression.ObjectType == (int)ObjectType.Active)
		{
			switch (expression.Num)
			{
				case 81: // XScale
					{
						if (expression.ObjectInfo == eventBase.ObjectInfo && expression.ObjectInfoList == eventBase.ObjectInfoList)
							return stringBuilder.Append("((Active*)instance)->xScale");
						else
							return stringBuilder.Append($"({GetSelector(expression.ObjectInfo)}->Count() > 0 ? ((Active*)*({GetSelector(expression.ObjectInfo)}->begin()))->xScale : 0)");
					}
				case 82: // YScale
					{
						if (expression.ObjectInfo == eventBase.ObjectInfo && expression.ObjectInfoList == eventBase.ObjectInfoList)
							return stringBuilder.Append("((Active*)instance)->yScale");
						else
							return stringBuilder.Append($"({GetSelector(expression.ObjectInfo)}->Count() > 0 ? ((Active*)*({GetSelector(expression.ObjectInfo)}->begin()))->yScale : 0)");
					}
				case 83: // Angle
					{
						if (expression.ObjectInfo == eventBase.ObjectInfo && expression.ObjectInfoList == eventBase.ObjectInfoList)
							return stringBuilder.Append("instance->GetAngle()");
						else
							return stringBuilder.Append($"({GetSelector(expression.ObjectInfo)}->Count() > 0 ? (*{GetSelector(expression.ObjectInfo)}->begin())->GetAngle() : 0)");
					}
			}
		}
		else if (expression.ObjectType == (int)ObjectType.Counter)
		{
			switch (expression.Num)
			{
				case 80: // Value
					return stringBuilder.Append($"({GetSelector(expression.ObjectInfo)}->Count() > 0 ? ((Counter*)*({GetSelector(expression.ObjectInfo)}->begin()))->GetValue() : 0)");
				case 82: // Max Value
					{
						if (expression.ObjectInfo == eventBase.ObjectInfo && expression.ObjectInfoList == eventBase.ObjectInfoList)
							return stringBuilder.Append($"((Counter*)instance)->MaxValue");
						else
							return stringBuilder.Append($"({GetSelector(expression.ObjectInfo)}->Count() > 0 ? ((Counter*)*({GetSelector(expression.ObjectInfo)}->begin()))->MaxValue : 0)");
					}
			}
		}
		else if (expression.ObjectType == (int)ObjectType.Text)
		{
			switch (expression.Num)
			{
				case 81: // String
					{
						if (expression.ObjectInfo == eventBase.ObjectInfo && expression.ObjectInfoList == eventBase.ObjectInfoList)
							return stringBuilder.Append("((StringObject*)instance)->GetText()");
						else
							return stringBuilder.Append($"({GetSelector(expression.ObjectInfo)}->Count() > 0 ? ((StringObject*)*({GetSelector(expression.ObjectInfo)}->begin()))->GetText() : std::string(\"\"))");
					}
				case 22: // Font Color
					return stringBuilder.Append("0");
			}
		}

		// if none of switch cases return than defaults to unimplemented
		HandleUnimplemented(stringBuilder, expression, eventBase);
		return stringBuilder;
	}
	private static void HandleUnimplemented(StringBuilder result, Expression expression, EventBase eventBase = null)
	{

		result.Append($"/* Expression not found: ({expression.ObjectType}, {expression.Num}) */");
		Logger.Log($"No expresion match, ObjectType: {expression.ObjectType}, Num: {expression.Num}");
	}

	public static string ConvertExpression(ExpressionParameter expressions, EventBase eventBase = null)
	{
		StringBuilder result = new();
		for (int i = 0; i < expressions.Items.Count; i++)
		{
			Expression expression = expressions.Items[i];

			if (expressionsLookup.TryGetValue(((ObjectType)expression.ObjectType, expression.Num), out var generator))
			{
				result.Append(generator(expression));
				continue;
			}

			switch ((ObjectType)expression.ObjectType)
			{
				case ObjectType.System:
					HandleSystemExpr(result, expression, eventBase); break; // for readablity sake, it is a seperate function

				default:
					if (expression.ObjectType > 0)
					{
						// Runtime Objects (Active, Text, Counter, etc.)
						HandleRuntimeObjectExpr(result, expression, eventBase);
					}
					else
					{
						HandleUnimplemented(result, expression, eventBase);
					}
					break;
			}
		}
		return result.ToString();
	}

	public static string GetComparisonSymbol(short comparison)
	{
		switch (comparison)
		{
			case 0: return "==";
			case 1: return "!=";
			case 2: return "<=";
			case 3: return "<";
			case 4: return ">=";
			case 5: return ">";
			default: return "==";
		}
	}

	public static string GetOppositeComparison(short comparison)
	{
		switch (comparison)
		{
			case 0: return "!=";
			case 1: return "==";
			case 2: return ">";
			case 3: return ">=";
			case 4: return "<";
			case 5: return "<=";
			default: return "!=";
		}
	}

	public static string GetSelector(int objectInfo, bool isGlobal = false)
	{
		var obj = GetObject(objectInfo, isGlobal);
		return $"{StringUtils.SanitizeObjectName(obj.Item2)}_{obj.Item1}_selector";
	}

	public static string GetObjectClassName(int objectInfo, bool isGlobal = false, bool convertToCCN = true)
	{
		int ObjectType = 0;
		int ObjectInfo = 0;
		if (convertToCCN)
		{
			var obj = GetObject(objectInfo, isGlobal);
			if (obj.Item1 >= short.MaxValue)
			{
				ObjectType = GetQualifierType(obj.Item2);
			}
			else
			{
				ObjectType = Exporter.Instance.GameData.frameitems[obj.Item1].ObjectType;
				ObjectInfo = obj.Item1;
			}
		}
		else
		{
			ObjectInfo = objectInfo;
			ObjectType = Exporter.Instance.GameData.frameitems[objectInfo].ObjectType;
		}

		switch (ObjectType)
		{
			case 0: return "QuickBackdrop";
			case 1: return "Backdrop";
			case 2: return "Active";
			case 3: return "StringObject";
			case 5: return "Score";
			case 6: return "Lives";
			case 7: return "Counter";
			case >= 32:
				ObjectCommon common = Exporter.Instance.GameData.frameitems[ObjectInfo].properties as ObjectCommon;
				ExtensionExporter exporter = ExtensionExporterRegistry.GetExporter(common.Identifier);
				return exporter?.CppClassName ?? "Extension";
			default: return "ObjectInstance";
		}
	}

	static int GetQualifierType(string qualifier)
	{
		string type = qualifier.Split('.')[1];
		switch (type)
		{
			case "Sprite": return 2;
			case "Text": return 3;
			case "Question": return 4;
			case "Score": return 5;
			case "Lives": return 6;
			case "Counter": return 7;
			default: return 32;
		}
	}

	public static Tuple<int, string, ObjectInstance> GetObject(int objectInfo, bool isGlobal = false, int frame = -1)
	{
		int FrameIndex = Exporter.Instance.CurrentFrame;
		if (frame != -1) FrameIndex = frame;

		string objectName = "";
		int objectType = 0;
		int systemQualifier = 0;
		ObjectInstance objectInstance = null;
		uint instanceHandle = 0;

		List<EventObject> eventObjects;
		if (isGlobal)
		{
			eventObjects = Exporter.Instance.MfaData.GlobalEvents.Objects;
		}
		else
		{
			eventObjects = Exporter.Instance.MfaData.Frames[FrameIndex].Events.Objects;
		}

		int mfaOIHandle = objectInfo;
		foreach (var evtObj in eventObjects)
		{
			if (evtObj.Handle == objectInfo)
			{
				mfaOIHandle = objectInfo;
				objectName = evtObj.Name;
				objectType = evtObj.ObjectType;
				systemQualifier = evtObj.SystemQualifier;
				instanceHandle = evtObj.InstanceHandle;

				//Find object name in ccn frame
				foreach (var ccnObj in Exporter.Instance.GameData.Frames[FrameIndex].objects)
				{
					if (objectName == Exporter.Instance.GameData.frameitems[(int)ccnObj.objectInfo].name)
					{
						objectInfo = ccnObj.objectInfo;
						objectInstance = ccnObj;
						break;
					}
				}
				break;
			}
		}

		if (systemQualifier != 0 ||
		(objectName == "Group.Player" && systemQualifier == 0 && instanceHandle == 0)) // temp fix since system qualifier returns 0 for the player group
		{
			Quailifer? ccnQualifier = Utilities.FindFrameQualifier(FrameIndex, mfaOIHandle, systemQualifier);
			if (ccnQualifier != null)
			{
				objectName = Utilities.GetQualifierName(ccnQualifier.Qualifier, ccnQualifier.Type);
			}
			else
			{
				objectName = Utilities.GetQualifierName(systemQualifier, objectType - 1);
			}
			
			objectInfo = short.MaxValue + systemQualifier + 1;
		}

		return new Tuple<int, string, ObjectInstance>(objectInfo, objectName, objectInstance);
	}
}
