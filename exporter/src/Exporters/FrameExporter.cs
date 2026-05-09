using System.Text;
using CTFAK.CCN.Chunks.Objects;
using CTFAK.CCN.Chunks.Frame;
using CTFAK.MMFParser.EXE.Loaders.Events.Parameters;
using CTFAK.MMFParser.EXE.Loaders.Events.Expressions;
using CTFAK.Utils;
using CTFAK.MMFParser.EXE.Loaders;
using System.Globalization;
using System.Drawing;

public class FrameExporter : BaseExporter
{
	private readonly ExpressionConverter _expressionConverter;
	private readonly EventProcessor _eventProcessor;

	public FrameExporter(Exporter exporter) : base(exporter)
	{
		_expressionConverter = new ExpressionConverter(exporter);
		_eventProcessor = new EventProcessor(exporter);
	}

	public override void Export()
	{
		var frameFactoryCppPath = Path.Combine(RuntimeBasePath.FullName, "source", "FrameFactory.template.cpp");

		// read templates
		var frameHeaderTemplate = File.ReadAllText(Path.Combine(RuntimeBasePath.FullName, "include", "Frame.template.h"));
		var frameCppTemplate = File.ReadAllText(Path.Combine(RuntimeBasePath.FullName, "source", "Frame.template.cpp"));
		var frameFactoryCppTemplate = File.ReadAllText(Path.Combine(RuntimeBasePath.FullName, "source", "FrameFactory.template.cpp"));

		string frameIncludes = "";
		string frameCases = "";

		for (int i = 0; i < GameData.Frames.Count; i++)
		{
			Logger.Log($"Exporting frame {i + 1}/{GameData.Frames.Count}...");
			_exporter.CurrentFrame = i;

			var frameHeader = frameHeaderTemplate.Replace("{{ FRAME_INDEX }}", i.ToString());
			var frameCpp = frameCppTemplate.Replace("{{ FRAME_INDEX }}", i.ToString());

			frameCpp = ProcessFrameTemplate(frameCpp, i);
			frameHeader = ProcessFrameHeader(frameHeader, i);

			// write frame files
			SaveFile(Path.Combine(OutputPath.FullName, "include", $"GeneratedFrame{i}.h"), frameHeader);
			SaveFile(Path.Combine(OutputPath.FullName, "source", $"GeneratedFrame{i}.cpp"), frameCpp);

			// add to factory
			frameIncludes += $"#include \"GeneratedFrame{i}.h\"\n";
			frameCases += $"        case {i}:\n            return std::make_unique<GeneratedFrame{i}>();\n";
		}

		// write factory implementation
		string frameFactoryCpp = frameFactoryCppTemplate.Replace("{{ FRAME_INCLUDES }}", frameIncludes);
		frameFactoryCpp = frameFactoryCpp.Replace("{{ FRAME_CASES }}", frameCases);
		frameFactoryCpp = frameFactoryCpp.Replace("{{ FRAME_COUNT }}", GameData.Frames.Count.ToString());

		SaveFile(Path.Combine(OutputPath.FullName, "source", "FrameFactory.cpp"), frameFactoryCpp);

		// delete the template files
		File.Delete(Path.Combine(OutputPath.FullName, "include", "Frame.template.h"));
		File.Delete(Path.Combine(OutputPath.FullName, "source", "Frame.template.cpp"));
		File.Delete(Path.Combine(OutputPath.FullName, "source", "FrameFactory.template.cpp"));
	}

	private string ProcessFrameTemplate(string frameCpp, int frameIndex)
	{
		var frame = GameData.Frames[frameIndex];

		frameCpp = frameCpp.Replace("{{ FRAME_INDEX }}", frameIndex.ToString());
		frameCpp = frameCpp.Replace("{{ FRAME_NAME }}", SanitizeString(frame.name));
		frameCpp = frameCpp.Replace("{{ FRAME_WIDTH }}", frame.width.ToString());
		frameCpp = frameCpp.Replace("{{ FRAME_HEIGHT }}", frame.height.ToString());
		frameCpp = frameCpp.Replace("{{ FRAME_BACKGROUND_COLOR }}", ColorToRGB(frame.background).ToString());

		frameCpp = frameCpp.Replace("{{ LAYER_INIT }}", BuildLayers(frame));
		frameCpp = frameCpp.Replace("{{ OBJECT_INSTANCES }}", BuildObjectInstances(frame));
		frameCpp = frameCpp.Replace("{{ OBJECT_SELECTORS_INIT }}", BuildObjectSelectorsInit(frame));
		frameCpp = frameCpp.Replace("{{ GROUP_ACTIVE }}", BuildGroupActive(frameIndex));

		_eventProcessor.PreProcessFrame(frameIndex);

		frameCpp = frameCpp.Replace("{{ EVENT_ANIMATION_UPDATE_LOOP }}", _eventProcessor.BuildEventUpdateLoop(frameIndex, EventProcessor.EventLoopType.Animation));
		frameCpp = frameCpp.Replace("{{ EVENT_TIMER_UPDATE_LOOP }}", _eventProcessor.BuildEventUpdateLoop(frameIndex, EventProcessor.EventLoopType.Timer));
		frameCpp = frameCpp.Replace("{{ EVENT_NORMAL_UPDATE_LOOP }}", _eventProcessor.BuildEventUpdateLoop(frameIndex, EventProcessor.EventLoopType.Normal));
		frameCpp = frameCpp.Replace("{{ EVENT_FUNCTIONS }}", _eventProcessor.BuildEventFunctions(frameIndex));

		return frameCpp;
	}

	private string ProcessFrameHeader(string frameHeader, int frameIndex)
	{
		var frame = GameData.Frames[frameIndex];

		frameHeader = frameHeader.Replace("{{ FRAME_INDEX }}", frameIndex.ToString());
		frameHeader = frameHeader.Replace("{{ OBJECT_SELECTORS }}", BuildObjectSelectors(frame));
		frameHeader = frameHeader.Replace("{{ EVENT_INCLUDES }}", _eventProcessor.BuildEventIncludes(frameIndex));
		frameHeader = frameHeader.Replace("{{ LOOP_INCLUDES }}", _eventProcessor.BuildLoopIncludes(frameIndex));
		frameHeader = frameHeader.Replace("{{ RUN_ONCE_CONDITION }}", _eventProcessor.BuildRunOnceCondition(frameIndex));
		frameHeader = frameHeader.Replace("{{ ONLY_ONE_ACTION_WHEN_LOOP_CONDITION }}", _eventProcessor.BuildOneActionLoop(frameIndex));

		return frameHeader;
	}

	private string BuildLayers(CTFAK.CCN.Chunks.Frame.Frame frame)
	{
		var layers = new StringBuilder();
		layers.AppendLine($"Layers.reserve({frame.layers.Items.Count});");
		foreach (var layer in frame.layers.Items)
		{
			string layerName = SanitizeObjectName(layer.Name);
			layers.AppendLine($"Layer {layerName} = Layer(\"{SanitizeString(layer.Name)}\", {layer.XCoeff}, {layer.YCoeff});");

			if (ColorUtils.ColorToRGB(layer.RGBCoeff) != "0xFFFFFFFF") layers.AppendLine($"{layerName}.RGBCoefficient = {ColorToRGB(layer.RGBCoeff)};");
			if (layer.RGBCoeff.A != 255) layers.AppendLine($"{layerName}.SetEffectParameter({Math.Clamp(byte.MaxValue - layer.RGBCoeff.A, 0, 255)});");
			
			if (layer.Flags.GetFlag("SameEffectAsPreviousLayer")) layers.AppendLine($"{layerName}.usePreviousLayerEffect = true;");
			if (layer.Effect != 0 && layer.Effect != 4096) layers.AppendLine($"{layerName}.Effect = {layer.Effect};");

			if (layer.shaderData.hasShader)
			{
				Shader shader = GameData.shaders.ShaderList[layer.shaderData.ShaderHandle];
				layers.AppendLine($"{layerName}.effectInstance = EffectBank::CreateEffect_{SanitizeObjectName(shader.Name)}_{layer.shaderData.ShaderHandle}();");
				for (int i = 0; i < layer.shaderData.parameters.Count; i++)
				{
					var parameter = shader.Parameters[i];
					string value;
					switch (parameter.Type)
					{
						case 0: //int, also used as bool
							value = $"static_cast<int>({BitConverter.ToInt32(layer.shaderData.parameters[i].Value as byte[], 0)})";
							break;
						case 1:
							//read float from bytes
							value = $"static_cast<float>({((float)BitConverter.ToSingle(layer.shaderData.parameters[i].Value as byte[], 0)).ToString(CultureInfo.InvariantCulture)}";
							if (!value.Contains('.')) value += ".0";
							value += "f)";
							break;
						case 2: //color
							value = $"static_cast<int>(0x{BitConverter.ToInt32(layer.shaderData.parameters[i].Value as byte[], 0):X8})";
							break;
						default:
							value = $"static_cast<int>({BitConverter.ToInt32(layer.shaderData.parameters[i].Value as byte[], 0)})";
							break;
					}
					layers.AppendLine($"{layerName}.effectInstance->SetParameter(\"{parameter.Name}\", {value});");
				}
			}

			layers.AppendLine($"Layers.push_back({layerName});");
		}
		return layers.ToString();
	}

	private string BuildObjectInstances(CTFAK.CCN.Chunks.Frame.Frame frame)
	{
		var objectInstances = new StringBuilder();
		var objectsCount = 0;
		foreach (var obj in frame.objects)
		{
			// skip instances not created on start
			if (obj.parentType != 0) continue;
			if (GameData.frameitems[(int)obj.objectInfo].properties is ObjectCommon common && common.Flags.GetFlag("DoNotCreateAtStart")) continue;
			string objectName = SanitizeObjectName(GameData.frameitems[(int)obj.objectInfo].name);
			objectInstances.Append($"CreateInstance(factory.CreateInstance_{objectName}_{obj.objectInfo}(), {obj.x}, {obj.y}, {obj.layer}, {obj.instance}, {obj.objectInfo}, 0, false);\n");
			objectsCount += 1;
		}

		if (objectsCount != 0) {
			objectInstances.Insert(0, $"ObjectInstances.reserve({objectsCount});\n");
			objectInstances.AppendLine($"MaxObjectInstanceHandle = {objectsCount};");
		}


		return objectInstances.ToString();
	}

	private string BuildObjectSelectors(CTFAK.CCN.Chunks.Frame.Frame frame)
	{
		var eventObjects = new StringBuilder();
		var uniqueHandles = new List<uint>();

		foreach (var obj in frame.objects)
		{
			if (!uniqueHandles.Contains(obj.objectInfo))
			{
				uniqueHandles.Add(obj.objectInfo);
				string objectName = GameData.frameitems[(int)obj.objectInfo].name;
				eventObjects.AppendLine($"std::shared_ptr<ObjectSelector> {SanitizeObjectName(objectName)}_{obj.objectInfo}_selector;");
			}
		}

		// qualifiers
		foreach (var qualifier in frame.events.QualifiersList)
		{
			if (!uniqueHandles.Contains((uint)qualifier.ObjectInfo))
			{
				uniqueHandles.Add((uint)qualifier.ObjectInfo);
				string objectName = Utilities.GetQualifierName(qualifier.Qualifier, qualifier.Type);
				eventObjects.AppendLine($"std::shared_ptr<ObjectSelector> {SanitizeObjectName(objectName)}_{qualifier.ObjectInfo}_selector;");
			}
		}

		return eventObjects.ToString();
	}

	private string BuildObjectSelectorsInit(CTFAK.CCN.Chunks.Frame.Frame frame)
	{
		var objectSelectorsInit = new StringBuilder();
		var uniqueHandles = new List<uint>();

		foreach (var obj in frame.objects)
		{
			if (!uniqueHandles.Contains(obj.objectInfo))
			{
				uniqueHandles.Add(obj.objectInfo);
				string objectName = GameData.frameitems[(int)obj.objectInfo].name;
				objectSelectorsInit.AppendLine($"{SanitizeObjectName(objectName)}_{obj.objectInfo}_selector = std::make_shared<ObjectSelector>(ObjectInstances, {obj.objectInfo}, false);");
			}
		}

		// qualifiers
		foreach (var qualifier in frame.events.QualifiersList)
		{
			if (!uniqueHandles.Contains((uint)qualifier.ObjectInfo))
			{
				uniqueHandles.Add((uint)qualifier.ObjectInfo);
				string objectName = Utilities.GetQualifierName(qualifier.ObjectInfo & 0x7FFF, qualifier.Type);
				objectSelectorsInit.AppendLine($"{SanitizeObjectName(objectName)}_{qualifier.ObjectInfo}_selector = std::make_shared<ObjectSelector>(ObjectInstances, {qualifier.ObjectInfo - 32768}, true);");
			}
		}

		return objectSelectorsInit.ToString();
	}

	private string BuildGroupActive(int frameIndex)
	{
		var groupActive = new StringBuilder();
		for (int j = 0; j < MfaData.Frames[frameIndex].Events.Items.Count; j++)
		{
			var evt = MfaData.Frames[frameIndex].Events.Items[j];
			if (evt.Conditions[0].ObjectType == -1 && evt.Conditions[0].Num == -10)
			{
				int groupId = (evt.Conditions[0].Items[0].Loader as Group).Id;
				bool isActiveOnStart = !(evt.Conditions[0].Items[0].Loader as Group).Flags.GetFlag("InactiveOnStart");
				groupActive.Append($"SetGroupActive({groupId}, {isActiveOnStart.ToString().ToLower()});\n");
			}
		}
		return groupActive.ToString();
	}
}
