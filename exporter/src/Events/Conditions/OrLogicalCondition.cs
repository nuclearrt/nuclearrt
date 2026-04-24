using System.Text;
using CTFAK.CCN.Chunks.Frame;

public class OrLogicalCondition : ConditionBase
{
	public override int ObjectType { get; set; } = -1;
	public override int Num { get; set; } = -25;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new();

		//if we got here then the conditions mustve succeeded, so we can skip to the actions
		result.AppendLine($"goto {parameters["eventBaseName"]}_actions;");

		result.AppendLine($"{parameters["eventBaseName"]}_or_{orIndex}:;");

		orIndex++;
		nextLabel = EventProcessor.GenerateEventNextLabel(parameters["eventGroup"] as EventGroup, orIndex);

		//Reset instances
		//TODO: check if were are only resetting actually relevant instances
		foreach (var relevantObjectInfo in EventProcessor.GetRelevantObjectInfos(parameters["eventGroup"] as EventGroup))
		{
			result.AppendLine($"{StringUtils.SanitizeObjectName(relevantObjectInfo.Item2)}_{relevantObjectInfo.Item1}_selector->Reset();");
		}

		return result.ToString();
	}
}
