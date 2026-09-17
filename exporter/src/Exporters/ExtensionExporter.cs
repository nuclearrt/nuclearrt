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
	private static readonly List<ExtensionExporter> exporters =
	[
		new ButtonObjectExporter(),
		new IniExporter(),
		new LayerObjectExporter(),
		new GlobalStoreXExporter(),
		new PerspectiveExporter(),
		new IniPlusPlusExporter(),
		new EasingObjectExporter(),
		new XBOXGamepadExporter(),
		new CrasherExporter(),
		new ArrayExporter(),
	];

	public static ExtensionExporter GetExporter(string extensionName)
	{
		return exporters.Find(e => e.CanHandle(extensionName));
	}

	public static ExtensionExporter? GetExporterByObjectInfo(int objectInfo, int frameIndex)
	{
		//get the object info from the frame
		var oi = ExpressionConverter.GetObject(objectInfo, frameIndex);

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

	public virtual bool IsTrueEvent(int conditionNum)
	{
		//this is kinda shit
		// extension actions/conditions should be written like any other action/condition
		return false;
	}

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

	protected string GetExtensionInstance(int objectInfo, int objectType)
	{
		string selector = GetSelector(objectInfo, objectType);
		return $"(({CppClassName}*)*({selector}.begin()))";
	}

	protected string GetExtensionInstanceLoop()
	{
		return $"(({CppClassName}*)instance)";
	}

	public string GetSelector(int objectInfo, int objectType)
	{
		return ExpressionConverter.GetSelector(objectInfo, objectType);
	}

	public string EvaluateExpression(EventBase eventBase, int num)
	{
		ExpressionParameter expressionParam = (ExpressionParameter)eventBase.Items[num].Loader;
		return ExpressionConverter.ConvertExpression(expressionParam, eventBase);
	}
}
