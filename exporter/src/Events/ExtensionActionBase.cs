using CTFAK.CCN.Chunks.Frame;
using CTFAK.Utils;

public class ExtensionActionBase : ActionBase
{
	public override string Build(EventBase eventBase, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (")
	{
		var exporter = ExtensionExporterRegistry.GetExporterByObjectInfo(eventBase.ObjectInfo, (int)parameters["frameIndex"]);
		if (exporter == null)
		{
			return $"// Extension exporter not found for ObjectInfo {eventBase.ObjectInfo}";
		}

		return exporter.ExportAction(eventBase, eventBase.Num - 80, ref nextLabel, ref orIndex, parameters, IsGlobal);
	}
}
