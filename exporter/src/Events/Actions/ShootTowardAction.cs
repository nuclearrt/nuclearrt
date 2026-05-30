using System.Text;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;
using CTFAK.Utils;

public class ShootTowardAction : ActionBase
{
	public override int[] ObjectType { get; set; } = [2];
	public override int Num { get; set; } = 30;

	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		StringBuilder result = new StringBuilder();

		Shoot shoot = (Shoot)eventBase.Items[0].Loader;
		Position position = (Position)eventBase.Items[1].Loader;
		var objectInfo = ExpressionConverter.GetObject(shoot.ObjectInfo, IsGlobal);

		result.AppendLine($"// {position.ToString()}");
		result.AppendLine($"for (ObjectIterator it(*{GetSelector(eventBase.ObjectInfo)}); !it.end(); ++it) {{");

		result.AppendLine($"    int targetX = {position.X};");
		result.AppendLine($"    int targetY = {position.Y};");

		if (position.ObjectInfoParent != ushort.MaxValue) // we are shooting toward an object
		{
			result.AppendLine($"    if ({GetSelector((int)position.ObjectInfoParent)}->Count() == 0) continue;");
			result.AppendLine($"    auto targetInstance = {GetSelector((int)position.ObjectInfoParent)}->begin();");
			if ((position.Flags & 0x2) != 0) // shoot toward actionpoint
			{
				result.AppendLine($"    targetX += ((Active*)*targetInstance)->GetXActionPoint();");
				result.AppendLine($"    targetY += ((Active*)*targetInstance)->GetYActionPoint();");
			}
			else // shoot toward x/y
			{
				result.AppendLine($"    targetX += ((Active*)*targetInstance)->X;");
				result.AppendLine($"    targetY += ((Active*)*targetInstance)->Y;");
			}

		}

		result.AppendLine($"    auto instance = *it;");
		result.AppendLine($"    ObjectInstance* newCreatedInstance = CreateInstance(ObjectFactory::Instance().CreateInstance_{StringUtils.SanitizeObjectName(objectInfo.Item2)}_{objectInfo.Item1}(), ((Active*)instance)->GetXActionPoint(), ((Active*)instance)->GetYActionPoint(), instance->Layer, 0, {objectInfo.Item1}, 0, true, nullptr);");
		result.AppendLine($"    {GetSelector(shoot.ObjectInfo)}->AddInstance(newCreatedInstance);");


		result.AppendLine($"    int shootDirection = 0;");
		result.AppendLine($"    double dx = static_cast<double>(targetX - newCreatedInstance->X);");
		result.AppendLine($"    double dy = static_cast<double>(targetY - newCreatedInstance->Y);");
		result.AppendLine($"    double angle = std::atan2(-dy, dx);");
		result.AppendLine($"    shootDirection = (int)std::round(angle * (32.0 / (2.0 * 3.14159265358979323846)));");
		result.AppendLine($"    shootDirection = shootDirection & 31;");

		result.AppendLine($"    ((Active*)newCreatedInstance)->movements.items.clear();");
		result.AppendLine($"    ((Active*)newCreatedInstance)->movements.items.insert(std::pair<int, Movement*>(0, new BulletMovement(0, true, (1 << shootDirection), {shoot.ShootSpeed})));");
		result.AppendLine($"    ((Active*)newCreatedInstance)->movements.GetCurrentMovement()->Instance = newCreatedInstance;");
		result.AppendLine($"    ((Active*)newCreatedInstance)->movements.GetCurrentMovement()->Initialize();");
		result.AppendLine($"    ((Active*)newCreatedInstance)->movements.GetCurrentMovement()->OnEnabled();");
		result.AppendLine("}");

		return result.ToString();
	}
}
