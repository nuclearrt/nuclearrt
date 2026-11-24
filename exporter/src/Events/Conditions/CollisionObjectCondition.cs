using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class CollisionObjectCondition : ConditionBase
{
	public override int[] ObjectType { get; set; } = [2, 7];
	public override int Num { get; set; } = -14;
	public override bool IsTrueEvent => true;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new();

		ParamObject obj = (ParamObject)eventBase.Items[0].Loader;

		string selector = GetSelector(eventBase.ObjectInfo, eventBase.ObjectType);
		string selector2 = GetSelector(obj.ObjectInfo, obj.ObjectType);

		result.AppendLine($"if (!trueEventSource || !trueEventOther) goto {nextLabel};");

		if (selector == selector2)
		{
			result.AppendLine($"if (trueEventSource->Handle > trueEventOther->Handle) goto {nextLabel};");
			result.AppendLine($"{selector}.RestoreSelection({{trueEventSource, trueEventOther}});");
			result.AppendLine($"if ({selector}.Count() != 2) goto {nextLabel};");
		}
		else
		{
			result.AppendLine($"{selector}.RestoreSelection({{trueEventSource}});");
			result.AppendLine($"{selector2}.RestoreSelection({{trueEventOther}});");
			result.AppendLine($"if ({selector}.Count() == 0 || {selector2}.Count() == 0) goto {nextLabel};");
		}

		return result.ToString();
	}
}
