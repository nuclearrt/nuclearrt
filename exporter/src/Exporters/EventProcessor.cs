using System.Reflection;
using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MFA;
using CTFAK.MMFParser.EXE.Loaders.Events.Expressions;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;
using CTFAK.Utils;

public class EventProcessor
{
	private readonly Exporter _exporter;

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

	public string BuildEventUpdateLoop(int frameIndex, bool isTimerUpdateLoop = false)
	{
		var result = new StringBuilder();

		for (int j = 0; j < _exporter.MfaData.Frames[frameIndex].Events.Items.Count; j++)
		{
			var evt = _exporter.MfaData.Frames[frameIndex].Events.Items[j];
			string eventName = GetEventName(evt);

			for (int k = 0; k < evt.RestrictCpt; k++) //TODO: check if this is correct
			{
				result.Append("\t");
			}

			// TODO: if a group is empty, don't include it.
			if (ShouldSkipEvent(evt))
			{
				if (new GroupStartCondition().Equals(evt.Conditions[0])) //if this event is a group start, don't include it in the main event update loop
				{
					result.Append($"if (IsGroupActive({(evt.Conditions[0].Items[0].Loader as Group).Id})) {{\n");
				}
				else if (new GroupEndCondition().Equals(evt.Conditions[0])) //if this event is a group end, don't include it in the main event update loop, just close the current group
				{
					result.Remove(result.Length - 1, 1); //Remove the last tab
					result.Append("}\n");
				}

				continue;
			}

			//only add event to normal event loop if it doesn't have a loop condition
			if (DoesEventHaveLoop(evt) == null && IsTimerEvent(evt) == isTimerUpdateLoop) result.Append($"{eventName}();\n");
		}
		return result.ToString();
	}

	public string BuildEventFunctions(int frameIndex)
	{
		var result = new StringBuilder();

		for (int j = 0; j < _exporter.MfaData.Frames[frameIndex].Events.Items.Count; j++)
		{
			var evt = _exporter.MfaData.Frames[frameIndex].Events.Items[j];

			if (ShouldSkipEvent(evt)) continue;

			result.AppendLine($"void GeneratedFrame{frameIndex}::{GetEventName(evt)}()");
			result.AppendLine("{");

			if (DoesEventHaveOneActionLoop(evt))
			{
				result.AppendLine($"bool allConditionsMet = false;");
			}

			int numberOfOrConditions = NumberOfOrConditions(evt);
			int orConditionIndex = 0;
			string nextLabel = GenerateEventNextLabel(evt, numberOfOrConditions);
			string idName = GetEventBaseName(evt);

			List<Tuple<int, string>> usedSelectors = new List<Tuple<int, string>>(); // if a selector has already been reset during this event, don't reset it again

			foreach (var condition in evt.Conditions)
			{
				//reset any selectors used in this condition if it wasn't reset in a previous condition
				foreach (var obj in GetRelevantObjectInfos(condition, frameIndex, evt.IsGlobal).Item2.Distinct().ToList())
				{
					if (usedSelectors.Any(x => x.Item1 == obj.Item1)) continue;
					usedSelectors.Add(obj);
					result.AppendLine($"{StringUtils.SanitizeObjectName(obj.Item2)}_{obj.Item1}_selector->Reset();");
				}

				var acBaseTypes = Assembly.GetExecutingAssembly().GetTypes().Where(t => t.IsSubclassOf(typeof(ConditionBase))).ToList();
				var acBaseType = acBaseTypes.FirstOrDefault(t =>
				{
					var instance = Activator.CreateInstance(t) as ConditionBase;
					return instance?.ObjectType == condition.ObjectType && instance?.Num == condition.Num;
				});

				if (condition.ObjectType >= 32)
				{
					acBaseType = typeof(ExtensionConditionBase);
				}

				if (acBaseType == null)
				{
					result.AppendLine($"//Condition ({condition.ObjectType}, {condition.Num}) not found");
					result.AppendLine($"goto {nextLabel};");
					continue;
				}

				Dictionary<string, object> parameters = new Dictionary<string, object>()
				{
					{ "eventIndex", j },
					{ "eventBaseName", idName }, // used for labels
					{ "frameIndex", frameIndex },
					{ "eventGroup", evt },
					{ "numOfOrs", numberOfOrConditions }
				};

				var instance = Activator.CreateInstance(acBaseType) as ConditionBase;
				instance.IsGlobal = evt.IsGlobal;
				string ifStatement = (condition.OtherFlags & 1) == 0 ? "if (!" : "if (";
				result.AppendLine(instance?.Build(condition, ref nextLabel, ref orConditionIndex, parameters, ifStatement));
			}

			string endLabel = $"{idName}_end";
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
				foreach (var obj in GetRelevantObjectInfos(action, frameIndex, evt.IsGlobal).Item2.Distinct().ToList())
				{
					if (usedSelectors.Any(x => x.Item1 == obj.Item1)) continue;
					usedSelectors.Add(obj);
					result.AppendLine($"{StringUtils.SanitizeObjectName(obj.Item2)}_{obj.Item1}_selector->Reset();");
				}

				var acBaseTypes = Assembly.GetExecutingAssembly().GetTypes().Where(t => t.IsSubclassOf(typeof(ActionBase))).ToList();
				var acBaseType = acBaseTypes.FirstOrDefault(t =>
				{
					var instance = Activator.CreateInstance(t) as ActionBase;
					return instance?.ObjectType == action.ObjectType && instance?.Num == action.Num;
				});

				if (action.ObjectType >= 32)
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
				instance.IsGlobal = evt.IsGlobal;
				result.AppendLine(instance?.Build(action, ref nextLabel, ref orConditionIndex, parameters, ""));
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
			for (int j = 0; j < _exporter.MfaData.Frames[frameIndex].Events.Items.Count; j++)
			{
				var evt = _exporter.MfaData.Frames[frameIndex].Events.Items[j];

				foreach (var condition in evt.Conditions)
				{
					if (!new LoopCondition().Equals(condition)) continue;

					string loopNameExpr = ExpressionConverter.ConvertExpression(condition.Items[0]?.Loader as ExpressionParameter);
					result.AppendLine($"\tif (LoopNameEquals(loopName, {loopNameExpr})) {GetEventName(evt)}();");
				}
			}

			result.AppendLine("}");
		}

		return result.ToString();
	}

	public static List<Tuple<int, string>> GetRelevantObjectInfos(EventGroup eventGroup)
	{
		List<Tuple<int, string>> relevantObjectInfos = new List<Tuple<int, string>>();

		foreach (var condition in eventGroup.Conditions)
		{
			relevantObjectInfos.AddRange(GetRelevantObjectInfos(condition, Exporter.Instance.CurrentFrame, eventGroup.IsGlobal).Item2);
		}

		foreach (var action in eventGroup.Actions)
		{
			relevantObjectInfos.AddRange(GetRelevantObjectInfos(action, Exporter.Instance.CurrentFrame, eventGroup.IsGlobal).Item2);
		}

		return relevantObjectInfos.Distinct().ToList();
	}

	//TODO: this is a mess
	//returns expected number of OIs and a list of all OIs used in this event condition or action
	public static Tuple<int, List<Tuple<int, string>>> GetRelevantObjectInfos(EventBase eventBase, int frameIndex, bool isGlobal = false)
	{
		int count = 0;
		List<Tuple<int, string>> relevantObjectInfos = new List<Tuple<int, string>>();

		List<int> objectInfos = new List<int>();

		if (eventBase.ObjectType > 0)
		{
			objectInfos.Add(eventBase.ObjectInfo);
			count++;
		}

		foreach (var expression in eventBase.Items)
		{
			if (expression.Loader is ExpressionParameter)
			{
				foreach (var exp in (expression.Loader as ExpressionParameter).Items)
				{
					if (exp.ObjectType > 0)
					{
						objectInfos.Add(exp.ObjectInfo);
						count++;
					}
				}
			}
			else if (expression.Loader is Position)
			{
				if ((expression.Loader as Position).ObjectInfoParent != ushort.MaxValue)
				{
					objectInfos.Add((int)(expression.Loader as Position).ObjectInfoParent);
					count++;
				}
			}
			else if (expression.Loader is ParamObject)
			{
				objectInfos.Add((expression.Loader as ParamObject).ObjectInfo);
				count++;
			}
			else if (expression.Loader is Create)
			{
				objectInfos.Add((expression.Loader as Create).ObjectInfo);
				count++;

				if ((expression.Loader as Create).Position.ObjectInfoParent != ushort.MaxValue)
				{
					objectInfos.Add((int)(expression.Loader as Create).Position.ObjectInfoParent);
					count++;
				}
			}
		}

		foreach (var objectInfo in objectInfos)
		{
			List<EventObject> eventObjects = new List<EventObject>();
			if (isGlobal)
				eventObjects = Exporter.Instance.MfaData.GlobalEvents.Objects;
			else
				eventObjects = Exporter.Instance.MfaData.Frames[frameIndex].Events.Objects;

			foreach (var evtObj in eventObjects)
			{
				if (evtObj.Handle == objectInfo)
				{
					string objectName = evtObj.Name;
					int objectType = evtObj.ObjectType;
					int systemQualifier = evtObj.SystemQualifier;

					if (systemQualifier != 0 ||
					(objectName == "Group.Player" && systemQualifier == 0 && evtObj.InstanceHandle == 0)) // temp fix since system qualifier returns 0 for the player group
					{
						relevantObjectInfos.Add(new Tuple<int, string>(short.MaxValue + systemQualifier + 1, Utilities.GetQualifierName(systemQualifier, objectType - 1)));
						break;
					}

					//Find object name in ccn frame
					foreach (var ccnObj in Exporter.Instance.GameData.Frames[frameIndex].objects)
					{
						if (objectName == Exporter.Instance.GameData.frameitems[(int)ccnObj.objectInfo].name)
						{
							relevantObjectInfos.Add(new Tuple<int, string>(ccnObj.objectInfo, objectName));
							break;
						}
					}
					break;
				}
			}
		}

		return new Tuple<int, List<Tuple<int, string>>>(count, relevantObjectInfos.Distinct().ToList());
	}


	public string BuildEventIncludes(int frameIndex)
	{
		var result = new StringBuilder();

		for (int j = 0; j < _exporter.MfaData.Frames[frameIndex].Events.Items.Count; j++)
		{
			var evt = _exporter.MfaData.Frames[frameIndex].Events.Items[j];

			if (ShouldSkipEvent(evt)) continue;

			result.AppendLine($"void {GetEventName(evt)}();");
		}

		return result.ToString();
	}

	public List<string> GetAllLoopNames(int frameIndex)
	{
		List<string> loopNames = new();

		foreach (var evt in _exporter.MfaData.Frames[frameIndex].Events.Items)
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

	private bool HasAnyLoopEvents(int frameIndex)
	{
		foreach (var evt in _exporter.MfaData.Frames[frameIndex].Events.Items)
		{
			if (DoesEventHaveLoop(evt) != null) return true;
		}
		return false;
	}

	public string BuildRunOnceCondition(int frameIndex)
	{
		StringBuilder result = new();

		for (int i = 0; i < _exporter.MfaData.Frames[frameIndex].Events.Items.Count; i++)
		{
			var evt = _exporter.MfaData.Frames[frameIndex].Events.Items[i];
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

		for (int i = 0; i < _exporter.MfaData.Frames[frameIndex].Events.Items.Count; i++)
		{
			var evt = _exporter.MfaData.Frames[frameIndex].Events.Items[i];
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
		foreach (var condition in evtGroup.Conditions)
		{
			if (new StartOfFrameCondition().Equals(condition)
				|| new TimerComparisonLessThanCondition().Equals(condition)
				|| new TimerComparisonGreaterThanCondition().Equals(condition)
				|| new TimerComparisonEqualToCondition().Equals(condition))
			{
				return true;
			}
		}

		return false;
	}

	//Adds global events to the frame
	public void PreProcessFrame(int frameIndex)
	{
		var frame = _exporter.MfaData.Frames[frameIndex];

		foreach (var evt in _exporter.MfaData.GlobalEvents.Items)
		{
			//check if this event has an object not present in the current frame, if so, don't add the event to the frame
			bool shouldSkipEvent = false;
			foreach (var condition in evt.Conditions)
			{
				if (GetRelevantObjectInfos(condition, frameIndex, true).Item1 != GetRelevantObjectInfos(condition, frameIndex, true).Item2.Count) // some objects are not present in the current frame
				{
					shouldSkipEvent = true;
					break;
				}
			}

			if (shouldSkipEvent) continue;

			evt.IsGlobal = true;
			frame.Events.Items.Add(evt);
		}
	}
	// utilities
	public static string GetEventBaseName(EventGroup evt)
	{
		return $"event{(evt.IsGlobal ? "_global" : "")}_{evt.Identifier}";
	}

	public static string GetEventName(EventGroup evt)
	{
		return $"Event{(evt.IsGlobal ? "_Global" : "")}_{evt.Identifier}";
	}

	public static string GenerateEventNextLabel(EventGroup evt, int orConditionIndex = 0)
	{
		string suffix = orConditionIndex > 0 ? $"_or_{orConditionIndex}" : "_end";
		return $"{GetEventBaseName(evt)}{suffix}";
	}
}
