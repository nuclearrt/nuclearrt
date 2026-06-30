using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class CenterDisplayAtAction : ActionBase
{
	public override int[] ObjectType { get; set; } = [-3];
	public override int Num { get; set; } = 7;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new StringBuilder();

		Position position = (Position)eventBase.Items[0].Loader;
		if (position.ObjectInfoParent == ushort.MaxValue) // Absolute position
		{
			result.AppendLine($"SetScroll({position.X}, {position.Y});");
		}
		else // Relative position from object
		{
			result.AppendLine($"for (ObjectIterator it(*{GetSelector((int)position.ObjectInfoParent, position.TypeParent)}); !it.end(); ++it) {{");
			result.AppendLine($"    auto parent = *it;");
			result.AppendLine($"    SetScroll({position.X} + parent->GetX(), {position.Y} + parent->GetY(), parent->Layer);");
			result.AppendLine("}");
		}

		return result.ToString();
	}
}
