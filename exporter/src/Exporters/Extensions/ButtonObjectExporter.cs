using CTFAK.Memory;
using CTFAK.CCN.Chunks.Frame;
using System.Text;
using CTFAK.Utils;

public class ButtonObjectExporter : ExtensionExporter
{
	public override string ObjectIdentifier => "0ntB";
	public override string ExtensionName => "kcbutton";
	public override string CppClassName => "ButtonObjectExtension";

	public override string ExportExtension(byte[] extensionData)
	{
		ByteReader reader = new ByteReader(extensionData);

		short Width = reader.ReadInt16();
		short Height = reader.ReadInt16();

		short Type = reader.ReadInt16();
		reader.ReadInt16();
		short Flags = reader.ReadInt16();

		return CreateExtension($"{Width}, {Height}, {Type}, {Flags}");
	}

	public override string ExportCondition(EventBase eventBase, int conditionNum, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (", bool isGlobal = false)
	{
		StringBuilder result = new StringBuilder();

		switch (conditionNum)
		{
			case 2: // "Button is clicked" condition
				result.AppendLine($"for (ObjectIterator it(*{ExpressionConverter.GetSelector(eventBase.ObjectInfo, isGlobal)}); !it.end(); ++it) {{");
				result.AppendLine($"    auto instance = *it;");
				result.AppendLine($"    {ifStatement} {GetExtensionInstanceLoop()}->IsClicked()) it.deselect();");
				result.AppendLine("}");
				result.AppendLine($"if ({ExpressionConverter.GetSelector(eventBase.ObjectInfo, isGlobal)}->Count() == 0) goto {nextLabel};");
				break;

			default:
				result.AppendLine($"// Button condition {conditionNum} not implemented");
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
			case 1: // Show button
				result.AppendLine($"for (ObjectIterator it(*{ExpressionConverter.GetSelector(eventBase.ObjectInfo, isGlobal)}); !it.end(); ++it) {{");
				result.AppendLine($"    auto instance = *it;");
				result.AppendLine($"    {GetExtensionInstanceLoop()}->SetShown(true);");
				result.AppendLine("}");
				break;
			case 2: // Hide button
				result.AppendLine($"for (ObjectIterator it(*{ExpressionConverter.GetSelector(eventBase.ObjectInfo, isGlobal)}); !it.end(); ++it) {{");
				result.AppendLine($"    auto instance = *it;");
				result.AppendLine($"    {GetExtensionInstanceLoop()}->SetShown(false);");
				result.AppendLine("}");
				break;
			case 3: // Enable button
				result.AppendLine($"for (ObjectIterator it(*{ExpressionConverter.GetSelector(eventBase.ObjectInfo, isGlobal)}); !it.end(); ++it) {{");
				result.AppendLine($"    auto instance = *it;");
				result.AppendLine($"    {GetExtensionInstanceLoop()}->SetEnabled(true);");
				result.AppendLine("}");
				break;
			case 4: // Disable button
				result.AppendLine($"for (ObjectIterator it(*{ExpressionConverter.GetSelector(eventBase.ObjectInfo, isGlobal)}); !it.end(); ++it) {{");
				result.AppendLine($"    auto instance = *it;");
				result.AppendLine($"    {GetExtensionInstanceLoop()}->SetEnabled(false);");
				result.AppendLine("}");
				break;
			default:
				result.AppendLine($"// Button action {actionNum} not implemented");
				break;
		}

		return result.ToString();
	}
}
