using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class MakeInvisibleAction : ActionBase
{
	public override int ObjectType { get; set; } = 2;
	public override int Num { get; set; } = 26;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new StringBuilder();

		result.AppendLine($"for (ObjectIterator it(*{GetSelector(eventBase.ObjectInfo)}); !it.end(); ++it) {{");
		result.AppendLine($"    auto instance = *it;");
		result.AppendLine($"    (({ExpressionConverter.GetObjectClassName(eventBase.ObjectInfo, IsGlobal)}*)instance)->Visible = {(eventBase.Num == 26 ? false : true).ToString().ToLower()};");
		result.AppendLine("}");

		return result.ToString();
	}
}

public class ReappearAction : MakeInvisibleAction
{
	public override int Num { get; set; } = 27;
}

public class CounterMakeInvisibleAction : MakeInvisibleAction
{
	public override int ObjectType { get; set; } = 7;
}

public class CounterReappearAction : ReappearAction
{
	public override int ObjectType { get; set; } = 7;
}

public class StringMakeInvisibleAction : MakeInvisibleAction
{
	public override int ObjectType { get; set; } = 3;
}

public class StringReappearAction : ReappearAction
{
	public override int ObjectType { get; set; } = 3;
}

public class ExtensionMakeInvisibleAction : MakeInvisibleAction
{
	public override int ObjectType { get; set; } = 32;
}

public class ExtensionReappearAction : ReappearAction
{
	public override int ObjectType { get; set; } = 32;
}