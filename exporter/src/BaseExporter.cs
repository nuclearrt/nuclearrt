using System.Drawing;
using CTFAK.CCN;
using CTFAK.MFA;

public abstract class BaseExporter
{
	protected readonly Exporter _exporter;

	protected GameData GameData => _exporter.GameData;
	protected MFAData MfaData => _exporter.MfaData;
	protected DirectoryInfo RuntimeBasePath => _exporter.RuntimeBasePath;
	protected DirectoryInfo OutputPath => _exporter.OutputPath;
	protected int CurrentFrame => _exporter.CurrentFrame;

	protected BaseExporter(Exporter exporter)
	{
		_exporter = exporter;
	}

	public abstract void Export();

	protected void SaveFile(string path, string content)
	{
		FileUtils.SaveFile(path, content);
	}

	protected string SanitizeString(string input)
	{
		return StringUtils.SanitizeString(input);
	}

	protected string SanitizeObjectName(string input)
	{
		return StringUtils.SanitizeObjectName(input);
	}

	protected string ColorToRGB(Color color)
	{
		return ColorUtils.ColorToRGB(color);
	}

	protected string ColorToArgb(Color color)
	{
		return ColorUtils.ColorToArgb(color);
	}
}
