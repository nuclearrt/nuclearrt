using System.Drawing;

public static class ColorUtils
{
	public static string ColorToRGB(Color color)
	{
		return $"0xFF{color.R:X2}{color.G:X2}{color.B:X2}";
	}

	public static string ColorToArgb(Color color)
	{
		return $"0x{color.A:X2}{color.R:X2}{color.G:X2}{color.B:X2}";
	}
}
