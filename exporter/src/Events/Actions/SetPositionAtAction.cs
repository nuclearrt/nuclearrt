using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class SetPositionAtAction : ActionBase
{
	public override int[] ObjectType { get; set; } = [2, 3, 7];
	public override int Num { get; set; } = 1;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new StringBuilder();

		result.AppendLine($"for (ObjectIterator it(*{GetSelector(eventBase.ObjectInfo)}); !it.end(); ++it) {{");
		result.AppendLine($"    auto instance = *it;");

		Position position = (Position)eventBase.Items[0].Loader;
		if (position.ObjectInfoParent == ushort.MaxValue) // Absolute position
		{
			result.AppendLine($"    instance->X = {position.X};");
			result.AppendLine($"    instance->Y = {position.Y};");
		}
		else // Relative position from object
		{
			//get the object
			result.AppendLine($"    auto parent = {GetSelector((int)position.ObjectInfoParent)}->At(it.index());");
			result.AppendLine($"    if (parent != nullptr) {{");
			result.AppendLine($"        instance->X = {position.X} + parent->X;");
			result.AppendLine($"        instance->Y = {position.Y} + parent->Y;");
			result.AppendLine($"    }}");
		}
		result.AppendLine("}");

		return result.ToString();
	}
}
