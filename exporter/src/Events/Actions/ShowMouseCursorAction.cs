using CTFAK.CCN.Chunks.Frame;

public class ShowMouseCursorAction : ActionBase
{
	public override int[] ObjectType { get; set; } = [-6];
	public override int Num { get; set; } = 1;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"Application::Instance().GetBackend()->input->ShowMouseCursor();";
	}
}
