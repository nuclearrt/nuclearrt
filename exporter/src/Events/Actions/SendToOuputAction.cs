using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class SendToOutputAction : ActionBase
{
	public override int ObjectType { get; set; } = -1;
	public override int Num { get; set; } = 42;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new StringBuilder();

		result.AppendLine("#ifdef _DEBUG");
		result.AppendLine($"	Application::Instance().GetBackend()->platform->Log({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");
		result.AppendLine("#endif");

		return result.ToString();
	}
}
