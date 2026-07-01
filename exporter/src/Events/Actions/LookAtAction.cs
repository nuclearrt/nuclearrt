using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;
using CTFAK.Utils;

public class LookAtAction : ActionBase
{
	public override int[] ObjectType { get; set; } = [2];
	public override int Num { get; set; } = 14;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new StringBuilder();

		result.AppendLine($"for (ObjectIterator it(*{GetSelector(eventBase.ObjectInfo, eventBase.ObjectType)}); !it.end(); ++it) {{");
		result.AppendLine($"    auto instance = *it;");
		Position position = (Position)eventBase.Items[0].Loader;
		if (position.ObjectInfoParent == ushort.MaxValue) {
			result.AppendLine($"    ((Active*)instance)->movements.GetCurrentMovement()->LookAtPoint({position.X}, {position.Y});");
		}
		else {
			result.AppendLine($"    if ({GetSelector((int)position.ObjectInfoParent, position.TypeParent)}->Count() > 0) {{");
			result.AppendLine($"        auto parent = *({GetSelector((int)position.ObjectInfoParent, position.TypeParent)}->begin());");
			result.AppendLine($"        ((Active*)instance)->movements.GetCurrentMovement()->LookAtObject(parent, {position.X}, {position.Y});");
			result.AppendLine($"    }}");
		}
		result.AppendLine("}");

		return result.ToString();
	}
}
