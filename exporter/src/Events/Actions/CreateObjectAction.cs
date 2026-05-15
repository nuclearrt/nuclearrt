using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class CreateObjectAction : ActionBase
{
	public override int ObjectType { get; set; } = -5;
	public override int Num { get; set; } = 0;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new();

		Create create = (Create)eventBase.Items[0].Loader;
		var objectInfo = ExpressionConverter.GetObject(create.ObjectInfo, IsGlobal);

		result.AppendLine("{");
		if (create.Position.ObjectInfoParent != ushort.MaxValue) // has parent
		{
			result.AppendLine($"for (ObjectIterator it(*{GetSelector((int)create.Position.ObjectInfoParent)}); !it.end(); ++it) {{");
			result.AppendLine($"    auto parent = *it;");
			result.AppendLine($"    ObjectInstance* newCreatedInstance = CreateInstance(ObjectFactory::Instance().CreateInstance_{StringUtils.SanitizeObjectName(objectInfo.Item2)}_{objectInfo.Item1}(), {create.Position.X}, {create.Position.Y}, {create.Position.Layer}, 0, {objectInfo.Item1}, {create.Position.Angle}, true, parent);");
			result.AppendLine($"    {GetSelector(create.ObjectInfo)}->AddInstance(newCreatedInstance);");
			result.AppendLine($"    {GetSelector(create.ObjectInfo)}->SelectOnly(newCreatedInstance);");
			result.AppendLine($"}}");
		}
		else
		{
			result.AppendLine($"ObjectInstance* instance = CreateInstance(ObjectFactory::Instance().CreateInstance_{StringUtils.SanitizeObjectName(objectInfo.Item2)}_{objectInfo.Item1}(), {create.Position.X}, {create.Position.Y}, {create.Position.Layer}, 0, {objectInfo.Item1}, {create.Position.Angle}, true);");
			//add to selector
			result.AppendLine($"{GetSelector(create.ObjectInfo)}->AddInstance(instance);");
			result.AppendLine($"{GetSelector(create.ObjectInfo)}->SelectOnly(instance);");
		}

		result.AppendLine("}");

		return result.ToString();
	}
}
