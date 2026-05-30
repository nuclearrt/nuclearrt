using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;

public class ShootAction : ActionBase
{
	public override int[] ObjectType { get; set; } = [2];
	public override int Num { get; set; } = 29;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new StringBuilder();

		Shoot shoot = (Shoot)eventBase.Items[0].Loader;
		var objectInfo = ExpressionConverter.GetObject(shoot.ObjectInfo, IsGlobal);
		bool followLauncherDirection = (shoot.ShootPos.Flags & 0x4) != 0;

		string direction = shoot.ShootPos.Direction.ToString();
		if (followLauncherDirection)
			direction = $"(1 << ((Active*)instance)->movements.GetCurrentMovement()->GetMovementDirection())";

		result.AppendLine($"for (ObjectIterator it(*{GetSelector(eventBase.ObjectInfo)}); !it.end(); ++it) {{");
		result.AppendLine($"    auto instance = *it;");
		result.AppendLine($"    ObjectInstance* newCreatedInstance = CreateInstance(ObjectFactory::Instance().CreateInstance_{StringUtils.SanitizeObjectName(objectInfo.Item2)}_{objectInfo.Item1}(), ((Active*)instance)->GetXActionPoint(), ((Active*)instance)->GetYActionPoint(), instance->Layer, 0, {objectInfo.Item1}, 0, true, nullptr);");
		result.AppendLine($"    {GetSelector(shoot.ObjectInfo)}->AddInstance(newCreatedInstance);");
		result.AppendLine($"    ((Active*)newCreatedInstance)->movements.items.clear();");
		result.AppendLine($"    ((Active*)newCreatedInstance)->movements.items.insert(std::pair<int, Movement*>(0, new BulletMovement(0, true, {direction}, {shoot.ShootSpeed})));");
		result.AppendLine($"    ((Active*)newCreatedInstance)->movements.GetCurrentMovement()->Instance = newCreatedInstance;");
		result.AppendLine($"    ((Active*)newCreatedInstance)->movements.GetCurrentMovement()->Initialize();");
		result.AppendLine($"    ((Active*)newCreatedInstance)->movements.GetCurrentMovement()->OnEnabled();");
		result.AppendLine("}");

		return result.ToString();
	}
}
