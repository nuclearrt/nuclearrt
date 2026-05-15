using System.Text;

public class AppDataExporter : BaseExporter
{
	public AppDataExporter(Exporter exporter) : base(exporter) { }

	public override void Export()
	{
		var appDataTemplatePath = Path.Combine(RuntimeBasePath.FullName, "source", "AppData.template.cpp");
		var appData = File.ReadAllText(appDataTemplatePath);

		appData = appData.Replace("{{ app_name }}", SanitizeString(GameData.name));
		appData = appData.Replace("{{ about_box }}", SanitizeString(GameData.aboutText));
		appData = appData.Replace("{{ window_width }}", GameData.header.WindowWidth.ToString());
		appData = appData.Replace("{{ window_height }}", GameData.header.WindowHeight.ToString());
		appData = appData.Replace("{{ target_fps }}", GameData.header.FrameRate.ToString());
		appData = appData.Replace("{{ border_color }}", ColorToRGB(GameData.header.BorderColor).ToString());
		appData = appData.Replace("{{ fit_inside }}", GameData.header.Flags.GetFlag("FitInsideBars") ? "true" : "false");
		appData = appData.Replace("{{ resize_display }}", GameData.header.Flags.GetFlag("ResizeDisplay") ? "true" : "false");
		appData = appData.Replace("{{ dont_center_frame }}", GameData.header.Flags.GetFlag("DontCenterFrame") ? "true" : "false");
		appData = appData.Replace("{{ anti_aliasing_when_resizing }}", GameData.ExtHeader.Flags.GetFlag("AntiAliasingWhenResizing") ? "true" : "false");
		appData = appData.Replace("{{ sample_over_frame }}", GameData.header.NewFlags.GetFlag("SamplesOverFrames") ? "true" : "false");
		appData = appData.Replace("{{ multi_samples }}", GameData.header.Flags.GetFlag("MultiSamples") ? "true" : "false");
		appData = appData.Replace("{{ sample_unfocused }}", GameData.header.NewFlags.GetFlag("PlaySamplesWhenUnfocused") ? "true" : "false" );

		appData = appData.Replace("{{ global_values }}", BuildGlobalValues());
		appData = appData.Replace("{{ global_strings }}", BuildGlobalStrings());
		appData = appData.Replace("{{ control_types }}", BuildControlTypes());
		appData = appData.Replace("{{ control_keys }}", BuildControlKeys());
		appData = appData.Replace("{{ player_scores }}", BuildPlayerScores());
		appData = appData.Replace("{{ player_lives }}", BuildPlayerLives());

		SaveFile(Path.Combine(OutputPath.FullName, "source", "AppData.cpp"), appData);
		File.Delete(Path.Combine(OutputPath.FullName, "source", "AppData.template.cpp"));
	}

	private string BuildGlobalValues()
	{
		var globalValues = new StringBuilder();
		globalValues.Append("{ ");
		if (GameData.globalValues != null)
		{
			foreach (var value in GameData.globalValues.Items)
			{
				globalValues.Append(value);
				if (value != GameData.globalValues.Items.Last())
				{
					globalValues.Append(", ");
				}
			}
		}
		globalValues.Append(" }");
		return globalValues.ToString();
	}

	private string BuildGlobalStrings()
	{
		var globalStrings = new StringBuilder();
		globalStrings.Append("{ ");
		if (GameData.globalStrings != null)
		{
			foreach (var str in GameData.globalStrings.Items)
			{
				globalStrings.Append($"\"{SanitizeString(str)}\"");
				if (str != GameData.globalStrings.Items.Last())
				{
					globalStrings.Append(", ");
				}
			}
		}
		globalStrings.Append(" }");
		return globalStrings.ToString();
	}

	private string BuildControlTypes()
	{
		var controlsTypes = new StringBuilder();
		controlsTypes.Append("{ ");
		foreach (var control in GameData.header.Controls.Items)
		{
			int type = control.ControlType;
			if (type == 5) type = 0;
			controlsTypes.Append($"{type}, ");
		}
		controlsTypes.Append("}");
		return controlsTypes.ToString();
	}

	private string BuildControlKeys()
	{
		var controlsKeys = new StringBuilder();
		controlsKeys.Append("{ ");
		foreach (var control in GameData.header.Controls.Items)
		{
			controlsKeys.Append($"{{ ");
			controlsKeys.Append($"{control.Keys.Up}, ");
			controlsKeys.Append($"{control.Keys.Down}, ");
			controlsKeys.Append($"{control.Keys.Left}, ");
			controlsKeys.Append($"{control.Keys.Right}, ");
			controlsKeys.Append($"{control.Keys.Button1}, ");
			controlsKeys.Append($"{control.Keys.Button2}, ");
			controlsKeys.Append($"{control.Keys.Button3}, ");
			controlsKeys.Append($"{control.Keys.Button4}");
			controlsKeys.Append("}, ");
		}
		controlsKeys.Append("}");
		return controlsKeys.ToString();
	}

	private string BuildPlayerScores()
	{
		var scores = new StringBuilder();
		scores.Append("{ ");
		for (int i = 0; i < 4; i++)
		{
			scores.Append($"{GameData.header.InitialScore}, ");
		}
		scores.Append("}");
		return scores.ToString();
	}

	private string BuildPlayerLives()
	{
		var lives = new StringBuilder();
		lives.Append("{ ");
		for (int i = 0; i < 4; i++)
		{
			lives.Append($"{GameData.header.InitialLives}, ");
		}
		lives.Append("}");
		return lives.ToString();
	}
}
