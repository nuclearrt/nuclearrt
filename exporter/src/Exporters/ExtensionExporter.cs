using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using CTFAK.CCN.Chunks.Objects;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;
using CTFAK.MMFParser.EXE.Loaders.Events.Expressions;

public static class ExtensionExporterRegistry
{
	private static readonly List<ExtensionExporter> exporters = new List<ExtensionExporter>
	{
		new ButtonObjectExporter(),
		new IniExporter(),
		new LayerObjectExporter(),
		new GlobalStoreXExporter(),
		new PerspectiveExporter()
	};

	public static ExtensionExporter GetExporter(string extensionName)
	{
		return exporters.Find(e => e.CanHandle(extensionName));
	}

	public static ExtensionExporter? GetExporterByObjectInfo(int objectInfo, int frameIndex)
	{
		//get the object info from the frame
		var oi = ExpressionConverter.GetObject(objectInfo, false, frameIndex);

		//get identifier from ccn
		ObjectInfo? obj = Exporter.Instance.GameData.frameitems.GetValueOrDefault(oi.Item1);
		if (obj != null && obj.properties is ObjectCommon common)
		{
			return GetExporter(common.Identifier);
		}

		return null;
	}
}

public abstract class ExtensionExporter
{
	public abstract string ObjectIdentifier { get; }
	public abstract string ExtensionName { get; }
	public abstract string CppClassName { get; }

	public abstract string ExportExtension(byte[] extensionData);

	public virtual string ExportCondition(EventBase eventBase, int conditionNum, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, string ifStatement = "if (", bool isGlobal = false)
	{
		return $"// Extension condition {ExtensionName}:{conditionNum} not implemented";
	}

	public virtual string ExportAction(EventBase eventBase, int actionNum, ref string nextLabel, ref int orIndex, Dictionary<string, object>? parameters = null, bool isGlobal = false)
	{
		return $"// Extension action {ExtensionName}:{actionNum} not implemented";
	}

	public virtual string ExportExpression(Expression expression, EventBase eventBase = null)
	{
		return $"0 /* Extension expression {ExtensionName}:{expression.Num} not implemented */";
	}

	public bool CanHandle(string extensionName)
	{
		return ObjectIdentifier.Equals(extensionName, StringComparison.OrdinalIgnoreCase);
	}

	protected string CreateExtension(string parameters)
	{
		return parameters;
	}

	protected string GetExtensionInstance(int objectInfo, bool isGlobal = false)
	{
		string selector = ExpressionConverter.GetSelector(objectInfo, isGlobal);
		return $"(({CppClassName}*)*({selector}->begin()))";
	}

	protected string GetExtensionInstanceLoop()
	{
		return $"(({CppClassName}*)instance)";
	}
}
