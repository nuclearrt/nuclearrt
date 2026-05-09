using CTFAK.CCN.Chunks.Frame;

public class ACBase
{
	public virtual int ObjectType { get; set; }
	public virtual int Num { get; set; }

	public virtual bool IsGlobal { get; set; } = false;

	//if statement and next label are only used for conditions
	public virtual string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		return $"//{GetType().Name} not implemented. ObjectType: {ObjectType}, Num: {Num}";
	}

	public string GetSelector(int objectInfo)
	{
		return ExpressionConverter.GetSelector(objectInfo, IsGlobal);
	}

	public override bool Equals(object? obj)
	{
		if (obj.GetType().IsSubclassOf(typeof(EventBase)))
		{
			EventBase eventBase = obj as EventBase;
			return (eventBase.ObjectType == ObjectType || eventBase.ObjectType >= 32 && ObjectType >= 32) && eventBase.Num == Num;
		}
		return base.Equals(obj);
	}
}