using CTFAK.Memory;
using CTFAK.CCN.Chunks.Frame;
using System.Text;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;
using CTFAK.CCN.Chunks;

public class EasingObjectExporter : ExtensionExporter
{
	public override string ObjectIdentifier => "GnsE";
	public override string ExtensionName => "Easing";
	public override string CppClassName => "EasingObjectExtension";

	public override string ExportExtension(byte[] extensionData)
	{
		ByteReader reader = new(extensionData);

		float overshoot = reader.ReadSingle();
		float amplitude = reader.ReadSingle();
		float period = reader.ReadSingle();

		return CreateExtension($"{overshoot:F}f, {amplitude:F}f, {period:F}f");
	}

	public override string ExportCondition(EventBase eventBase, int conditionNum, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (", bool isGlobal = false)
	{
		StringBuilder result = new StringBuilder();

		switch (conditionNum)
		{
			default:
				result.AppendLine($"// Easing condition {conditionNum} not implemented");
				result.AppendLine($"goto {nextLabel};");
				break;
		}

		return result.ToString();
	}

	public override string ExportAction(EventBase eventBase, int actionNum, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, bool isGlobal = false)
	{
		StringBuilder result = new StringBuilder();

		switch (actionNum)
		{
			case 0: // Move Object
				result.AppendLine($"for (ObjectIterator it(*{GetSelector(((ParamObject)eventBase.Items[0].Loader).ObjectInfo, isGlobal)}); !it.end(); ++it) {{");
				result.AppendLine($"    auto instance = *it;");
				result.AppendLine($"	{GetExtensionInstance(eventBase.ObjectInfo)}->MoveObject(instance, {ReadEasingParam(eventBase.Items[1].Loader)}, {EvaluateExpression(eventBase, 2)}, {EvaluateExpression(eventBase, 3)}, {ReadTimeModeParam(eventBase.Items[4].Loader)}, {EvaluateExpression(eventBase, 5)});");
				result.AppendLine("}");
				break;
			case 4: // Set Overshoot
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SetOvershoot({EvaluateExpression(eventBase, 0)});");
				break;
			case 5: // Set Amplitude
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SetAmplitude({EvaluateExpression(eventBase, 0)});");
				break;
			case 6: // Set Period
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->SetPeriod({EvaluateExpression(eventBase, 0)});");
				break;
			case 10: // Move Object Explicit
				result.AppendLine($"{GetExtensionInstance(eventBase.ObjectInfo)}->MoveObjectExplicit({EvaluateExpression(eventBase, 0)}, {EvaluateExpression(eventBase, 1)}, {EvaluateExpression(eventBase, 2)}, {EvaluateExpression(eventBase, 3)}, {EvaluateExpression(eventBase, 4)}, {EvaluateExpression(eventBase, 5)}, {EvaluateExpression(eventBase, 6)}, {EvaluateExpression(eventBase, 7)});");
				break;
			default:
				result.AppendLine($"// Easing action {actionNum} not implemented");
				break;
		}

		return result.ToString();
	}

	string ReadEasingParam(ChunkLoader loader)
	{
		CTFAK.MMFParser.EXE.Loaders.Events.Parameters.Extension extentionParam = (CTFAK.MMFParser.EXE.Loaders.Events.Parameters.Extension)loader;
		byte version = extentionParam.Data[0]; // not needed?
		byte method = extentionParam.Data[1];
		byte firstFunction = extentionParam.Data[2];
		byte secondFunction = extentionParam.Data[3];

		return $"{method}, {firstFunction}, {secondFunction}";
	}

	string ReadTimeModeParam(ChunkLoader loader)
	{
		CTFAK.MMFParser.EXE.Loaders.Events.Parameters.Extension extentionParam = (CTFAK.MMFParser.EXE.Loaders.Events.Parameters.Extension)loader;
		byte timeMode = extentionParam.Data[0];

		return $"{timeMode}";
	}
}
