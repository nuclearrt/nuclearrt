using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class CreateObjectAtAction : ActionBase
{
	public override int ObjectType { get; set; } = -5;
	public override int Num { get; set; } = 2;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new();

		ParamObject obj = (ParamObject)eventBase.Items[0].Loader;
		string X = ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase);
		string Y = ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[2].Loader, eventBase);
		string layer = ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[3].Loader, eventBase);
		var objectInfo = ExpressionConverter.GetObject(obj.ObjectInfo, IsGlobal);
		result.AppendLine("{");
		result.AppendLine($"ObjectInstance* instance = CreateInstance(ObjectFactory::Instance().CreateInstance_{StringUtils.SanitizeObjectName(objectInfo.Item2)}_{objectInfo.Item1}(), {X}, {Y}, ({layer}) - 1, 0, {objectInfo.Item1}, 0, true);");
		//add to selector
		result.AppendLine($"{GetSelector(obj.ObjectInfo)}->AddInstance(instance);");
		result.AppendLine($"{GetSelector(obj.ObjectInfo)}->SelectOnly(instance);");
		result.AppendLine("}");

		return result.ToString();
	}
}
