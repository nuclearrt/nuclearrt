using System.Text;
using CTFAK.CCN.Chunks.Frame;

public class RunOnceCondition : ConditionBase
{
	public override int ObjectType { get; set; } = -1;
	public override int Num { get; set; } = -6;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new();

		result.AppendLine($"if ({parameters["eventBaseName"]}_run_once) goto {nextLabel};");
		result.AppendLine($"{parameters["eventBaseName"]}_run_once = true;");

		return result.ToString();
	}
}
