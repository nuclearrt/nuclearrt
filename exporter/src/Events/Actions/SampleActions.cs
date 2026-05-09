using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;
public class PlaySample : ActionBase
{
	public override int ObjectType { get; set; } = -2;
	public override int Num { get; set; } = 0;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new();
		result.AppendLine($"Application::Instance().GetBackend()->audio->PlaySample({CheckType.Check(eventBase)}, -1, 1, NULL, {CheckType.GetUninterruptable(eventBase)}, -1, -2);");

		return result.ToString();
	}
}

public class PlaySampleAllParameters : ActionBase
{
	public override int ObjectType { get; set; } = -2;
	public override int Num { get; set; } = 36;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"Application::Instance().GetBackend()->audio->PlaySample({CheckType.Check(eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[2].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[5].Loader, eventBase)}, {CheckType.GetUninterruptable(eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[3].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[4].Loader, eventBase)});";
	}
}
public class CheckType
{
	public static string Check(EventBase eventBase)
	{
		string type;
		string val;
		if (eventBase.Items[0].Loader is Sample)
			val = $"\"{((Sample)eventBase.Items[0].Loader).Name}\"";
		else
			val = $"{ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}"; // Supposedly the String expression that uses the name of the sample.
		type = $"Application::Instance().GetBackend()->audio->FindSample({val})";
		return type;
	}
	public static string CheckFile(EventBase eventBase)
	{
		string val;
		if (eventBase.Items[0].Loader is Filename)
			val = $"\"{((Filename)eventBase.Items[0].Loader)}\"";
		else
			val = $"{ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}";
		return val;
	}
	public static string GetUninterruptable(EventBase eventBase)
	{
		string uninterruptable = "false";
		switch (((Sample)eventBase.Items[0].Loader).Flags)
		{
			case 1: // Play sample all params
			case 9: // Play sample
				uninterruptable = "true";
				break;
			case 0:
			case 8:
			default:
				uninterruptable = "false";
				break;
		}
		return uninterruptable;
	}
}
public class PlaySampleChannel : ActionBase
{
	public override int ObjectType { get; set; } = -2;
	public override int Num { get; set; } = 11;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new();

		result.AppendLine($"Application::Instance().GetBackend()->audio->PlaySample({CheckType.Check(eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)}, 1, NULL, {CheckType.GetUninterruptable(eventBase)}, -1, -2);");

		return result.ToString();
	}
}
public class PlayAndLoopSample : ActionBase
{
	public override int ObjectType { get; set; } = -2;
	public override int Num { get; set; } = -2;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new();
		result.AppendLine($"Application::Instance().GetBackend()->audio->PlaySample({CheckType.Check(eventBase)}, -1, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)}, NULL, {CheckType.GetUninterruptable(eventBase)}, -1, -2);");
		return result.ToString();
	}
}
public class StopAnySample : ActionBase
{
	public override int ObjectType { get; set; } = -2;
	public override int Num { get; set; } = 1;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return "Application::Instance().GetBackend()->audio->StopSample(-1, false);\n";
	}
}
public class PlayAndLoopSampleAtChannel : ActionBase
{
	public override int ObjectType { get; set; } = -2;
	public override int Num { get; set; } = 12;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new();
		result.AppendLine($"Application::Instance().GetBackend()->audio->PlaySample({CheckType.Check(eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[2].Loader, eventBase)}, NULL, {CheckType.GetUninterruptable(eventBase)}, -1, -2);");

		return result.ToString();
	}
}
public class SetMainVolume : ActionBase
{
	public override int ObjectType { get; set; } = -2;
	public override int Num { get; set; } = 20;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"Application::Instance().GetBackend()->audio->SetSampleVolume({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, -1, false);";
	}
}
public class SetChannelVolume : ActionBase
{
	public override int ObjectType { get; set; } = -2;
	public override int Num { get; set; } = 17;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"Application::Instance().GetBackend()->audio->SetSampleVolume({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, true);";
	}
}
public class SetSampleVolume : ActionBase
{
	public override int ObjectType { get; set; } = -2;
	public override int Num { get; set; } = 21;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"Application::Instance().GetBackend()->audio->SetSampleVolume({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)}, {CheckType.Check(eventBase)}, true);";
	}
}
public class PauseSpecificSample : ActionBase
{
	public override int ObjectType { get; set; } = -2;
	public override int Num { get; set; } = 7;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"Application::Instance().GetBackend()->audio->PauseSample({CheckType.Check(eventBase)}, false, {(eventBase.Num == 7 ? "true" : "false")});";
	}

}
public class ResumeSpecificSample : PauseSpecificSample
{
	public override int Num { get; set; } = 8;
}
public class PauseChannel : ActionBase
{
	public override int ObjectType { get; set; } = -2;
	public override int Num { get; set; } = 13;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"Application::Instance().GetBackend()->audio->PauseSample({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, true, {(eventBase.Num == 13 ? "true" : "false")});";
	}
}
public class ResumeChannel : PauseChannel
{
	public override int Num { get; set; } = 14;
}
public class PauseAllSamples : ActionBase
{
	public override int Num { get; set; } = 24;
	public override int ObjectType { get; set; } = -2;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"Application::Instance().GetBackend()->audio->PauseSample(-1, false, {(eventBase.Num == 24 ? "true" : "false")});";
	}
}
public class ResumeAllSamples : PauseAllSamples
{
	public override int Num { get; set; } = 25;
}
public class UnlockChannel : ActionBase
{
	public override int ObjectType { get; set; } = -2;
	public override int Num { get; set; } = 30;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"Application::Instance().GetBackend()->audio->LockChannel({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, {(eventBase.Num == 30 ? "false" : "true")});";
	}
}
public class LockChannel : UnlockChannel
{
	public override int Num { get; set; } = 31;
}
public class SetMainPan : ActionBase
{
	public override int ObjectType { get; set; } = -2;
	public override int Num { get; set; } = 22;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"Application::Instance().GetBackend()->audio->SetSamplePan({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, -1, false);";
	}
}
public class SetSamplePan : ActionBase
{
	public override int ObjectType { get; set; } = -2;
	public override int Num { get; set; } = 23;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"Application::Instance().GetBackend()->audio->SetSamplePan({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)}, {CheckType.Check(eventBase)}, false);";
	}
}

public class SetChannelPan : ActionBase
{
	public override int ObjectType { get; set; } = -2;
	public override int Num { get; set; } = 18;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"Application::Instance().GetBackend()->audio->SetSamplePan({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, true);";
	}
}
public class SetSampleFrequency : ActionBase
{
	public override int ObjectType { get; set; } = -2;
	public override int Num { get; set; } = 33;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"Application::Instance().GetBackend()->audio->SetSampleFreq({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)}, {CheckType.Check(eventBase)}, false);";
	}
}
public class SetChannelFrequency : ActionBase
{
	public override int ObjectType { get; set; } = -2;
	public override int Num { get; set; } = 32;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"Application::Instance().GetBackend()->audio->SetSampleFreq({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, true);";
	}
}
public class SetChannelPos : ActionBase
{
	public override int ObjectType { get; set; } = -2;
	public override int Num { get; set; } = 16;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"Application::Instance().GetBackend()->audio->SetSamplePos({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)} * 22, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, true);";
	}
}
public class SetSamplePos : ActionBase
{
	public override int ObjectType { get; set; } = -2;
	public override int Num { get; set; } = 19;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"Application::Instance().GetBackend()->audio->SetSamplePos({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)} * 22, {CheckType.Check(eventBase)}, false);";
	}
}
public class PreloadSampleFile : ActionBase
{
	public override int ObjectType { get; set; } = -2;
	public override int Num { get; set; } = 34;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"Application::Instance().GetBackend()->audio->{(eventBase.Num == 34 ? "LoadSampleFile" : "DiscardSampleFile")}({CheckType.CheckFile(eventBase)});";
	}
}
public class DiscardSampleFile : PreloadSampleFile
{
	public override int Num { get; set; } = 35;
}
public class PlaySampleFileChannel : ActionBase
{
	public override int ObjectType { get; set; } = -2;
	public override int Num { get; set; } = 28;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new StringBuilder();
		result.AppendLine($"Application::Instance().GetBackend()->audio->LoadSampleFile({CheckType.CheckFile(eventBase)});");
		result.AppendLine($"Application::Instance().GetBackend()->audio->PlaySampleFile({CheckType.CheckFile(eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)}, 1);");
		return result.ToString();
	}
}
public class PlaySampleFileChannelLoop : ActionBase
{
	public override int ObjectType { get; set; } = -2;
	public override int Num { get; set; } = 29;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new StringBuilder();
		result.AppendLine($"Application::Instance().GetBackend()->audio->LoadSampleFile({CheckType.CheckFile(eventBase)});");
		result.AppendLine($"Application::Instance().GetBackend()->audio->PlaySampleFile({CheckType.CheckFile(eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[1].Loader, eventBase)}, {ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[2].Loader, eventBase)});");
		return result.ToString();
	}
}
