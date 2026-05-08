using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;
public class NoSamplePlaying : ConditionBase
{
	public override int ObjectType { get; set; } = -2;
	public override int Num { get; set; } = -3;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"{ifStatement} (Application::Instance().GetBackend()->audio->SampleState(-1, false, false))) goto {nextLabel};";
	}
}
public class SpecificSampleNotPlaying : ConditionBase
{
	public override int ObjectType { get; set; } = -2;
	public override int Num { get; set; } = -1;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"{ifStatement} (Application::Instance().GetBackend()->audio->SampleState({((Sample)eventBase.Items[0].Loader).Handle}, false, false))) goto {nextLabel};";
	}
}
public class SpecificSamplePaused : ConditionBase
{
	public override int ObjectType { get; set; } = -2;
	public override int Num { get; set; } = -6;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"{ifStatement} (Application::Instance().GetBackend()->audio->SampleState({((Sample)eventBase.Items[0].Loader).Handle}, false, true))) goto {nextLabel};";
	}
}
public class ChannelNotPlaying : ConditionBase
{
	public override int ObjectType { get; set; } = -2;
	public override int Num { get; set; } = -8;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"{ifStatement} (Application::Instance().GetBackend()->audio->SampleState({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, true, false))) goto {nextLabel};";
	}
}
public class ChannelPaused : ConditionBase
{
	public override int ObjectType { get; set; } = -2;
	public override int Num { get; set; } = -9;
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"{ifStatement} (Application::Instance().GetBackend()->audio->SampleState({ExpressionConverter.ConvertExpression((ExpressionParameter)eventBase.Items[0].Loader, eventBase)}, true, true))) goto {nextLabel};";
	}
}
