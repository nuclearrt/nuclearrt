using System.Reflection;
using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.CCN.Chunks.Objects;
using CTFAK.MFA;
using CTFAK.MMFParser.EXE.Loaders.Events.Expressions;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;
using CTFAK.Utils;

public class EventProcessor
{
	private readonly Exporter _exporter;

	//child event shit
	private readonly Dictionary<int, List<int>> _directChildIndices = new();
	private readonly Dictionary<int, int> _parentOf = new();
	private readonly Dictionary<int, List<Tuple<int, int, string>>> _oisPassedToChildren = new();
	private readonly Dictionary<int, List<Tuple<int, int, string>>> _oisInheritedFromParent = new();

	public enum EventLoopType
	{
		Timer, // TODO: get rid of this, make them true events
		Normal,
	}

	public EventProcessor(Exporter exporter)
	{
		_exporter = exporter;
	}

	public bool ShouldSkipEvent(EventGroup evt)
	{
		if ((evt.Flags & 16384) != 0) return true; // Inactive line

		foreach (var condition in evt.Conditions)
		{
			if (new CommentCondition().Equals(condition) || new GroupStartCondition().Equals(condition) || new GroupEndCondition().Equals(condition)) return true;
		}

		return false;
	}

	public string BuildEventUpdateLoop(int frameIndex, EventLoopType eventLoopType)
	{
		var result = new StringBuilder();

		for (int j = 0; j < _exporter.GameData.Frames[frameIndex].events.Items.Count; j++)
		{
			var evt = _exporter.GameData.Frames[frameIndex].events.Items[j];
			string eventName = GetEventName(evt);

			// TODO: if a group is empty, don't include it.
			if (ShouldSkipEvent(evt))
			{
				if (new GroupStartCondition().Equals(evt.Conditions[0])) //if this event is a group start, don't include it in the main event update loop
				{
					result.Append($"if (IsGroupActive({(evt.Conditions[0].Items[0].Loader as Group).Id})) {{\n");
				}
				else if (new GroupEndCondition().Equals(evt.Conditions[0])) //if this event is a group end, don't include it in the main event update loop, just close the current group
				{
					result.Append("}\n");
				}

				continue;
			}

			if (IsChildEvent(j)) continue;

			//only add event to normal event loop if it doesn't have a loop condition
			if (DoesEventHaveLoop(evt) == null && DoesEventHaveNamedTimerEvent(evt) == null && !IsTrueEvent(evt, frameIndex))
			{
				if (eventLoopType == EventLoopType.Timer)
				{
					if (!IsTimerEvent(evt)) continue;
				}
				else if (eventLoopType == EventLoopType.Normal)
				{
					if (IsTimerEvent(evt)) continue;
				}

				result.Append($"{eventName}();\n");
			}
		}
		return result.ToString();
	}

	public string BuildEventFunctions(int frameIndex)
	{
		var result = new StringBuilder();

		for (int j = 0; j < _exporter.GameData.Frames[frameIndex].events.Items.Count; j++)
		{
			var evt = _exporter.GameData.Frames[frameIndex].events.Items[j];

			if (ShouldSkipEvent(evt)) continue;

			result.AppendLine($"void GeneratedFrame{frameIndex}::{GetEventName(evt)}()");
			result.AppendLine("{");

			if (DoesEventHaveOneActionLoop(evt))
			{
				result.AppendLine($"bool allConditionsMet = false;");
			}

			int numberOfOrConditions = NumberOfOrConditions(evt);
			int orConditionIndex = 0;
			string nextLabel = GenerateEventNextLabel(evt, 0, numberOfOrConditions);
			string idName = GetEventBaseName(evt);
			List<Tuple<int, int, string>> inheritedSelectors = _oisInheritedFromParent.GetValueOrDefault(j) ?? [];
			List<int> childIndices = _directChildIndices.GetValueOrDefault(j) ?? [];
			bool hasChildren = childIndices.Count > 0;
			string endLabel = $"{idName}_end";

			List<Tuple<int, int, string>> usedSelectors = [.. inheritedSelectors]; // if a selector has already been reset during this event, don't reset it again

			if (numberOfOrConditions > 0)
			{
				foreach (var inherited in inheritedSelectors)
				{
					result.AppendLine($"std::vector<ObjectInstance*> __entry_{StringUtils.SanitizeObjectName(inherited.Item3)}_{inherited.Item1} = {StringUtils.SanitizeObjectName(inherited.Item3)}_{inherited.Item1}_selector.SaveSelection();");
				}
			}

			foreach (var condition in evt.Conditions)
			{
				//reset any selectors used in this condition if it wasn't reset in a previous condition
				foreach (var obj in GetRelevantObjectInfos(condition).Distinct().ToList())
				{
					if (usedSelectors.Any(x => x.Item1 == obj.Item1 && x.Item2 == obj.Item2)) continue;
					usedSelectors.Add(obj);
					result.AppendLine($"{StringUtils.SanitizeObjectName(obj.Item3)}_{obj.Item1}_selector.Reset(trueEventSource);");
				}

				var acBaseTypes = Assembly.GetExecutingAssembly().GetTypes().Where(t => t.IsSubclassOf(typeof(ConditionBase))).ToList();
				var acBaseType = acBaseTypes.FirstOrDefault(t =>
				{
					var instance = Activator.CreateInstance(t) as ConditionBase;
					return instance != null && (instance.ObjectType.Contains(condition.ObjectType) || (condition.ObjectType >= 32 && instance.ObjectType.Any(x => x >= 32))) && instance.Num == condition.Num;
				});

				if (acBaseType == null && condition.ObjectType >= 32)
				{
					acBaseType = typeof(ExtensionConditionBase);
				}

				if (acBaseType == null)
				{
					result.AppendLine($"//Condition ({condition.ObjectType}, {condition.Num}) not found");
					result.AppendLine($"goto {nextLabel};");
					continue;
				}

				//TODO: move these to a struct
				Dictionary<string, object> parameters = new Dictionary<string, object>()
				{
					{ "eventIndex", j },
					{ "eventBaseName", idName }, // used for labels
					{ "frameIndex", frameIndex },
					{ "eventGroup", evt },
					{ "numOfOrs", numberOfOrConditions },
					{ "negatedIfStatement", (condition.OtherFlags & 1) == 0 ? "if (" : "if (!" },
					{ "inheritedSelectors", inheritedSelectors }
				};

				var instance = Activator.CreateInstance(acBaseType) as ConditionBase;
				string ifStatement = (condition.OtherFlags & 1) == 0 ? "if (!" : "if (";
				try
				{
					result.AppendLine(instance?.Build(condition, ref nextLabel, ref orConditionIndex, parameters, ifStatement));
				}
				catch (Exception)
				{
					Logger.Log($"Error building condition on {GetEventName(evt)}: {condition.ObjectType}, {condition.Num}");
					throw;
				}
			}

			result.AppendLine($"{idName}_actions:;");

			if (DoesEventHaveOneActionLoop(evt))
			{
				result.AppendLine($"allConditionsMet = true;");
				result.AppendLine($"if ({idName}_actions_executed_last_frame) goto {endLabel};");
				result.AppendLine($"{idName}_actions_executed_last_frame = true;");
			}

			foreach (var action in evt.Actions)
			{
				//reset any selectors used in this action if it wasn't reset in a previous action
				foreach (var obj in GetRelevantObjectInfos(action).Distinct().ToList())
				{
					if (usedSelectors.Any(x => x.Item1 == obj.Item1 && x.Item2 == obj.Item2)) continue;
					usedSelectors.Add(obj);
					result.AppendLine($"{StringUtils.SanitizeObjectName(obj.Item3)}_{obj.Item1}_selector.Reset(trueEventSource);");
				}

				var acBaseTypes = Assembly.GetExecutingAssembly().GetTypes().Where(t => t.IsSubclassOf(typeof(ActionBase))).ToList();
				var acBaseType = acBaseTypes.FirstOrDefault(t =>
				{
					var instance = Activator.CreateInstance(t) as ActionBase;
					return instance != null && (instance.ObjectType.Contains(action.ObjectType) || (action.ObjectType >= 32 && instance.ObjectType.Any(x => x >= 32))) && instance.Num == action.Num;
				});

				if (acBaseType == null && action.ObjectType >= 32)
				{
					acBaseType = typeof(ExtensionActionBase);
				}

				if (acBaseType == null)
				{
					result.AppendLine($"//Action ({action.ObjectType}, {action.Num}) not found");
					continue;
				}

				Dictionary<string, object> parameters = new Dictionary<string, object>()
				{
					{ "eventIndex", j },
					{ "frameIndex", frameIndex },
					{ "eventGroup", evt },
					{ "numOfOrs", numberOfOrConditions }
				};

				var instance = Activator.CreateInstance(acBaseType) as ActionBase;
				try
				{
					result.AppendLine(instance?.Build(action, ref nextLabel, ref orConditionIndex, parameters, ""));
				}
				catch (Exception)
				{
					Logger.Log($"Error building action on {GetEventName(evt)}: {action.ObjectType}, {action.Num}");
					throw;
				}
			}

			if (hasChildren)
			{
				result.AppendLine("{");
				result.Append(BuildChildEventCalls(frameIndex, j, childIndices));
				result.AppendLine("}");
			}

			result.AppendLine($"{endLabel}:;");

			if (DoesEventHaveOneActionLoop(evt))
			{
				string eventBaseName = GetEventBaseName(evt);
				result.AppendLine($"if (!allConditionsMet) {eventBaseName}_actions_executed_last_frame = false;");
			}

			result.AppendLine("}");
			result.AppendLine("");
		}

		//Create any loop functions
		if (HasAnyLoopEvents(frameIndex))
		{
			result.AppendLine($"void GeneratedFrame{frameIndex}::OnLoop(const std::string& loopName)");
			result.AppendLine("{");
			for (int j = 0; j < _exporter.GameData.Frames[frameIndex].events.Items.Count; j++)
			{
				var evt = _exporter.GameData.Frames[frameIndex].events.Items[j];
				if (ShouldSkipEvent(evt) || IsChildEvent(j)) continue;

				foreach (var condition in evt.Conditions)
				{
					if (!new LoopCondition().Equals(condition)) continue;

					string loopNameExpr = ExpressionConverter.ConvertExpression(condition.Items[0]?.Loader as ExpressionParameter);
					result.AppendLine($"\tif (LoopNameEquals(loopName, {loopNameExpr})) {GetEventName(evt)}();");
				}
			}

			result.AppendLine("}");
		}

		if (HasAnyNamedTimerEvents(frameIndex))
		{
			result.AppendLine($"void GeneratedFrame{frameIndex}::OnTimerEvent(const std::string& eventName)");
			result.AppendLine("{");
			for (int j = 0; j < _exporter.GameData.Frames[frameIndex].events.Items.Count; j++)
			{
				var evt = _exporter.GameData.Frames[frameIndex].events.Items[j];
				if (ShouldSkipEvent(evt) || IsChildEvent(j)) continue;

				foreach (var condition in evt.Conditions)
				{
					if (!new TimerEventCondition().Equals(condition)) continue;

					string eventNameExpr = ExpressionConverter.ConvertExpression(condition.Items[0]?.Loader as ExpressionParameter);
					result.AppendLine($"\tif (LoopNameEquals(eventName, {eventNameExpr})) {GetEventName(evt)}();");
				}
			}

			result.AppendLine("}");
		}

		if (HasAnyTrueEvents(frameIndex))
		{
			result.Append(BuildGenerateEventFunction(frameIndex));
		}

		return result.ToString();
	}

	public static List<Tuple<int, int, string>> GetRelevantObjectInfos(EventGroup eventGroup)
	{
		List<Tuple<int, int, string>> relevantObjectInfos = [];

		foreach (var condition in eventGroup.Conditions)
		{
			relevantObjectInfos.AddRange(GetRelevantObjectInfos(condition));
		}

		foreach (var action in eventGroup.Actions)
		{
			relevantObjectInfos.AddRange(GetRelevantObjectInfos(action));
		}

		return relevantObjectInfos.Distinct().ToList();
	}

	//TODO: this is a mess
	//returns list of all OIs used in this event condition or action
	public static List<Tuple<int, int, string>> GetRelevantObjectInfos(EventBase eventBase)
	{
		List<Tuple<int, int, string>> relevantObjectInfos = [];

		//object info, object type
		List<Tuple<int, int>> objectInfos = [];

		if (eventBase.ObjectType > 0)
		{
			objectInfos.Add(new Tuple<int, int>(eventBase.ObjectInfo, eventBase.ObjectType));
		}

		foreach (var expression in eventBase.Items)
		{
			if (expression.Loader is ExpressionParameter)
			{
				foreach (var exp in (expression.Loader as ExpressionParameter).Items)
				{
					if (exp.ObjectType > 0)
					{
						objectInfos.Add(new Tuple<int, int>(exp.ObjectInfo, exp.ObjectType));
					}
				}
			}
			else if (expression.Loader is Position)
			{
				if ((expression.Loader as Position).ObjectInfoParent != ushort.MaxValue)
				{
					objectInfos.Add(new Tuple<int, int>((int)(expression.Loader as Position).ObjectInfoParent, (expression.Loader as Position).TypeParent));
				}
			}
			else if (expression.Loader is ParamObject)
			{
				objectInfos.Add(new Tuple<int, int>((expression.Loader as ParamObject).ObjectInfo, (expression.Loader as ParamObject).ObjectType));
			}
			else if (expression.Loader is Create)
			{
				var create = (Create)expression.Loader;
				objectInfos.Add(new Tuple<int, int>(create.ObjectInfo, GetObjectType(create.ObjectInfo)));

				if (create.Position.ObjectInfoParent != ushort.MaxValue)
				{
					objectInfos.Add(new Tuple<int, int>((int)create.Position.ObjectInfoParent, create.Position.TypeParent));
				}
			}
			else if (expression.Loader is Shoot)
			{
				var shoot = (Shoot)expression.Loader;
				objectInfos.Add(new Tuple<int, int>(shoot.ObjectInfo, GetObjectType(shoot.ObjectInfo)));

				if (shoot.ShootPos.ObjectInfoParent != ushort.MaxValue)
				{
					objectInfos.Add(new Tuple<int, int>((int)shoot.ShootPos.ObjectInfoParent, shoot.ShootPos.TypeParent));
				}
			}
		}

		foreach (var objectInfo in objectInfos)
		{
			if (objectInfo.Item1 > short.MaxValue) // qualifier
			{
				string qualifierName = Utilities.GetQualifierName(objectInfo.Item1 & 0x7FFF, objectInfo.Item2);
				relevantObjectInfos.Add(new Tuple<int, int, string>(objectInfo.Item1, objectInfo.Item2, qualifierName));
			}
			else
			{
				ObjectInfo evtObj = Exporter.Instance.GameData.frameitems[objectInfo.Item1];
				string objectName = evtObj.name;

				relevantObjectInfos.Add(new Tuple<int, int, string>(objectInfo.Item1, evtObj.ObjectType, objectName));
			}
		}

		return [.. relevantObjectInfos.Distinct().ToList()];
	}

	static int GetObjectType(int objectInfo)
	{
		if (objectInfo > short.MaxValue) return 0;
		return Exporter.Instance.GameData.frameitems[objectInfo].ObjectType;
	}

	public string BuildEventIncludes(int frameIndex)
	{
		var result = new StringBuilder();

		for (int j = 0; j < _exporter.GameData.Frames[frameIndex].events.Items.Count; j++)
		{
			var evt = _exporter.GameData.Frames[frameIndex].events.Items[j];

			if (ShouldSkipEvent(evt)) continue;

			result.AppendLine($"void {GetEventName(evt)}();");
		}

		return result.ToString();
	}

	public List<string> GetAllLoopNames(int frameIndex)
	{
		List<string> loopNames = new();

		foreach (var evt in _exporter.GameData.Frames[frameIndex].events.Items)
		{
			string? loopName = DoesEventHaveLoop(evt);
			if (loopName != null)
			{
				loopNames.Add(StringUtils.SanitizeObjectName(loopName));
			}
		}

		return loopNames.Distinct().ToList();
	}

	//returns the loop name if the event has a loop condition, otherwise returns null
	string? DoesEventHaveLoop(EventGroup evtGroup)
	{
		foreach (var condition in evtGroup.Conditions)
		{
			if (new LoopCondition().Equals(condition)) return ExpressionConverter.ConvertExpression(condition.Items[0]?.Loader as ExpressionParameter).ToString() ?? "";
		}

		return null;
	}

	string? DoesEventHaveNamedTimerEvent(EventGroup evtGroup)
	{
		foreach (var condition in evtGroup.Conditions)
		{
			if (new TimerEventCondition().Equals(condition)) return ExpressionConverter.ConvertExpression(condition.Items[0]?.Loader as ExpressionParameter).ToString() ?? "";
		}

		return null;
	}

	bool DoesEventHaveRunOnce(EventGroup evtGroup)
	{
		foreach (var condition in evtGroup.Conditions)
		{
			if (new RunOnceCondition().Equals(condition)) return true;
		}

		return false;
	}

	int NumberOfOrConditions(EventGroup evtGroup)
	{
		int count = 0;
		foreach (var condition in evtGroup.Conditions)
		{
			if (new OrLogicalCondition().Equals(condition)) count++;
		}

		return count;
	}

	public string BuildLoopIncludes(int frameIndex)
	{
		if (!HasAnyLoopEvents(frameIndex)) return "";

		return "void OnLoop(const std::string& loopName) override;\n";
	}

	public string BuildTimerEventIncludes(int frameIndex)
	{
		if (!HasAnyNamedTimerEvents(frameIndex)) return "";

		return "void OnTimerEvent(const std::string& eventName) override;\n";
	}

	public string BuildTrueEventInclude(int frameIndex)
	{
		if (!HasAnyTrueEvents(frameIndex)) return "";

		return "void GenerateEvent(int objectType, int conditionNum, ObjectInstance* source = nullptr) override;\n";
	}

	private bool HasAnyLoopEvents(int frameIndex)
	{
		foreach (var evt in _exporter.GameData.Frames[frameIndex].events.Items)
		{
			if (DoesEventHaveLoop(evt) != null) return true;
		}
		return false;
	}

	private bool HasAnyNamedTimerEvents(int frameIndex)
	{
		foreach (var evt in _exporter.GameData.Frames[frameIndex].events.Items)
		{
			if (DoesEventHaveNamedTimerEvent(evt) != null) return true;
		}
		return false;
	}

	private bool HasAnyTrueEvents(int frameIndex)
	{
		foreach (var evt in _exporter.GameData.Frames[frameIndex].events.Items)
		{
			if (IsTrueEvent(evt, frameIndex)) return true;
		}
		return false;
	}

	public bool IsTrueEvent(EventGroup evtGroup, int frameIndex)
	{
		if (evtGroup.Conditions.Count == 0) return false;
		if (ShouldSkipEvent(evtGroup)) return false;
		if (DoesEventHaveLoop(evtGroup) != null) return false;
		if (DoesEventHaveNamedTimerEvent(evtGroup) != null) return false;
		return IsConditionTrueEvent(evtGroup.Conditions[0], frameIndex);
	}

	public bool IsConditionTrueEvent(EventBase condition, int frameIndex)
	{
		if (condition.ObjectType >= 32)
		{
			var exporter = ExtensionExporterRegistry.GetExporterByObjectInfo(condition.ObjectInfo, frameIndex);
			return exporter != null && exporter.IsTrueEvent(Math.Abs(condition.Num) - 80);
		}

		var handler = GetConditionHandler(condition);
		return handler != null && handler.IsTrueEvent;
	}

	ConditionBase? GetConditionHandler(EventBase condition)
	{
		var acBaseTypes = Assembly.GetExecutingAssembly().GetTypes().Where(t => t.IsSubclassOf(typeof(ConditionBase))).ToList();
		var acBaseType = acBaseTypes.FirstOrDefault(t =>
		{
			var instance = Activator.CreateInstance(t) as ConditionBase;
			return instance != null && (instance.ObjectType.Contains(condition.ObjectType) || (condition.ObjectType >= 32 && instance.ObjectType.Any(x => x >= 32))) && instance.Num == condition.Num;
		});

		if (acBaseType == null && condition.ObjectType >= 32)
		{
			acBaseType = typeof(ExtensionConditionBase);
		}

		if (acBaseType == null) return null;
		return Activator.CreateInstance(acBaseType) as ConditionBase;
	}

	string BuildGenerateEventFunction(int frameIndex)
	{
		var result = new StringBuilder();
		result.AppendLine($"void GeneratedFrame{frameIndex}::GenerateEvent(int objectType, int conditionNum, ObjectInstance* source)");
		result.AppendLine("{");
		result.AppendLine("\ttrueEventSource = source;");

		var groupStack = new List<int>();
		var items = _exporter.GameData.Frames[frameIndex].events.Items;

		for (int j = 0; j < items.Count; j++)
		{
			var evt = items[j];
			if (new GroupStartCondition().Equals(evt.Conditions[0]))
			{
				groupStack.Add((evt.Conditions[0].Items[0].Loader as Group).Id);
				continue;
			}
			if (new GroupEndCondition().Equals(evt.Conditions[0]))
			{
				if (groupStack.Count > 0) groupStack.RemoveAt(groupStack.Count - 1);
				continue;
			}

			if (IsChildEvent(j) || !IsTrueEvent(evt, frameIndex)) continue;

			var first = evt.Conditions[0];
			result.Append($"\tif (objectType == {first.ObjectType} && conditionNum == {first.Num}");
			if (first.ObjectType > 0)
			{
				result.Append(" && source != nullptr");
				if (first.ObjectInfo > short.MaxValue)
				{
					result.Append($" && source->Type == {first.ObjectType} && source->HasQualifier({first.ObjectInfo & 0x7FFF})");
				}
				else
				{
					result.Append($" && source->ObjectInfoHandle == {first.ObjectInfo}");
				}
			}
			result.AppendLine(") {");

			string indent = "\t\t";
			foreach (var groupId in groupStack)
			{
				result.AppendLine($"{indent}if (IsGroupActive({groupId})) {{");
				indent += "\t";
			}

			result.AppendLine($"{indent}{GetEventName(evt)}();");

			for (int g = groupStack.Count - 1; g >= 0; g--)
			{
				indent = indent.Substring(0, indent.Length - 1);
				result.AppendLine($"{indent}}}");
			}

			result.AppendLine("\t}");
		}

		result.AppendLine("\ttrueEventSource = nullptr;");
		result.AppendLine("}");
		result.AppendLine("");
		return result.ToString();
	}

	public string BuildRunOnceCondition(int frameIndex)
	{
		StringBuilder result = new();

		for (int i = 0; i < _exporter.GameData.Frames[frameIndex].events.Items.Count; i++)
		{
			var evt = _exporter.GameData.Frames[frameIndex].events.Items[i];
			if (DoesEventHaveRunOnce(evt))
			{
				string idName = GetEventBaseName(evt);
				result.AppendLine($"bool {idName}_run_once = false;");
			}
		}

		return result.ToString();
	}

	public bool DoesEventHaveOneActionLoop(EventGroup evtGroup)
	{
		foreach (var condition in evtGroup.Conditions)
		{
			if (new OneActionLoopCondition().Equals(condition)) return true;
		}

		return false;
	}

	public string BuildOneActionLoop(int frameIndex)
	{
		StringBuilder result = new();

		for (int i = 0; i < _exporter.GameData.Frames[frameIndex].events.Items.Count; i++)
		{
			var evt = _exporter.GameData.Frames[frameIndex].events.Items[i];
			if (DoesEventHaveOneActionLoop(evt))
			{
				string idName = GetEventBaseName(evt);
				result.AppendLine($"bool {idName}_actions_executed_last_frame = false;");
			}
		}

		return result.ToString();
	}

	// if true, this event should be called before the normal events
	public bool IsTimerEvent(EventGroup evtGroup)
	{
		// TODO: Verify if any other conditions should be considered a timer event, I got these from 2006 documentation: https://www.clickteam.com/creation_materials/tutorials/download/Fusion_runtime.pdf
		if (new TimerComparisonLessThanCondition().Equals(evtGroup.Conditions[0])
			|| new TimerComparisonGreaterThanCondition().Equals(evtGroup.Conditions[0])
			|| new TimerComparisonEqualToCondition().Equals(evtGroup.Conditions[0]))
		{
			return true;
		}

		return false;
	}

	public void PreProcessFrame(int frameIndex)
	{
		_directChildIndices.Clear();
		_parentOf.Clear();
		_oisPassedToChildren.Clear();
		_oisInheritedFromParent.Clear();

		var frame = _exporter.GameData.Frames[frameIndex];
		if (frame.events?.Items == null) return;

		var items = frame.events.Items;

		for (int i = 0; i < items.Count; i++)
		{
			var evt = items[i];
			if (ShouldSkipEvent(evt))
			{
				_directChildIndices[i] = [];
				_oisPassedToChildren[i] = [];
				continue;
			}

			var children = ComputeDirectChildIndices(i, items);
			_directChildIndices[i] = children;

			foreach (int childIndex in children)
			{
				_parentOf[childIndex] = i;
			}

			var passedOis = GetChildEventObjectInfos(evt);
			if (passedOis.Count == 0 && children.Count > 0)
			{
				passedOis = GetRelevantObjectInfos(evt);
			}
			_oisPassedToChildren[i] = passedOis;
		}

		for (int i = 0; i < items.Count; i++)
		{
			if (_parentOf.TryGetValue(i, out int parentIndex))
				_oisInheritedFromParent[i] = _oisPassedToChildren[parentIndex];
			else
				_oisInheritedFromParent[i] = [];
		}
	}

	string BuildChildEventCalls(int frameIndex, int parentIndex, List<int> childIndices)
	{
		var result = new StringBuilder();
		var items = _exporter.GameData.Frames[frameIndex].events.Items;
		var passedSelectors = _oisPassedToChildren.GetValueOrDefault(parentIndex) ?? [];

		foreach (var selector in passedSelectors)
		{
			result.AppendLine($"std::vector<ObjectInstance*> __child_{StringUtils.SanitizeObjectName(selector.Item3)}_{selector.Item1} = {StringUtils.SanitizeObjectName(selector.Item3)}_{selector.Item1}_selector.SaveSelection();");
		}

		foreach (int childIndex in childIndices)
		{
			foreach (var selector in passedSelectors)
			{
				result.AppendLine($"{StringUtils.SanitizeObjectName(selector.Item3)}_{selector.Item1}_selector.RestoreSelection(__child_{StringUtils.SanitizeObjectName(selector.Item3)}_{selector.Item1});");
			}
			result.AppendLine($"{GetEventName(items[childIndex])}();");
		}

		return result.ToString();
	}

	List<int> ComputeDirectChildIndices(int parentIndex, List<EventGroup> items)
	{
		int parentDepth = items[parentIndex].RestrictCpt;
		var children = new List<int>();

		for (int i = parentIndex + 1; i < items.Count; i++)
		{
			int depth = items[i].RestrictCpt;
			if (depth <= parentDepth) break;

			if (depth == parentDepth + 1 && !ShouldSkipEvent(items[i]) && (items[i].Flags & 0x8000) != 0)
			{
				children.Add(i);
			}
		}

		return children;
	}

	static List<Tuple<int, int, string>> GetChildEventObjectInfos(EventGroup evt)
	{
		var result = new List<Tuple<int, int, string>>();

		foreach (var condition in evt.Conditions)
		{
			foreach (var param in condition.Items)
			{
				if (param.Loader is not ChildEvent childEvent || childEvent.ois == null) continue;

				for (int i = 0; i + 1 < childEvent.ois.Length; i += 2)
				{
					var resolved = ResolveChildEventOi((ushort)childEvent.ois[i], childEvent.ois[i + 1]);
					if (resolved != null) result.Add(resolved);
				}
			}
		}

		return result.Distinct().ToList();
	}

	static Tuple<int, int, string>? ResolveChildEventOi(int objectInfo, int second)
	{
		if (objectInfo == ushort.MaxValue) return null;

		if (objectInfo > short.MaxValue)
		{
			int objectType = second > 0 ? second : 2;
			string qualifierName = Utilities.GetQualifierName(objectInfo & 0x7FFF, objectType);
			return new Tuple<int, int, string>(objectInfo, objectType, qualifierName);
		}

		if (!Exporter.Instance.GameData.frameitems.TryGetValue(objectInfo, out ObjectInfo? evtObj) || evtObj == null)
		{
			return null;
		}

		return new Tuple<int, int, string>(objectInfo, evtObj.ObjectType, evtObj.name);
	}

	// utilities
	public static string GetEventBaseName(EventGroup evt)
	{
		int index = Exporter.Instance.GameData.Frames[Exporter.Instance.CurrentFrame].events.Items.IndexOf(evt);
		return $"event_{index}";
	}

	public static string GetEventName(EventGroup evt)
	{
		int index = Exporter.Instance.GameData.Frames[Exporter.Instance.CurrentFrame].events.Items.IndexOf(evt);
		return $"Event_{index}";
	}

	public static string GenerateEventNextLabel(EventGroup evt, int orConditionIndex, int totalOrs)
	{
		if (orConditionIndex < totalOrs) return $"{GetEventBaseName(evt)}_or_{orConditionIndex}";
		return $"{GetEventBaseName(evt)}_end";
	}

	bool IsChildEvent(int eventIndex)
	{
		return _parentOf.ContainsKey(eventIndex);
	}
}
