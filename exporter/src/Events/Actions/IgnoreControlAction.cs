using CTFAK.CCN.Chunks.Frame;

public class IgnoreControlAction : ActionBase
{
	public override int ObjectType { get; set; } = -7;
	public override int Num { get; set; } = 2;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"Application::Instance().GetInput()->IgnoreControl({eventBase.ObjectInfo});";
	}
}
