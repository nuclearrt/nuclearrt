using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class CenterDisplayXAction : ActionBase
{
	public override int[] ObjectType { get; set; } = [-3];
	public override int Num { get; set; } = 8;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new StringBuilder();

		eventBase.ObjectInfoList = -1; // TODO: im doing this because or else it will write it as an "SetScrollX(instance->GetX())" rather than "SetScrollX(player_selector->begin()->X)"
		result.AppendLine($"SetScroll{(eventBase.Num == 8 ? "X" : "Y")}({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)});");

		return result.ToString();
	}
}

public class CenterDisplayYAction : CenterDisplayXAction
{
	public override int Num { get; set; } = 9;
}
