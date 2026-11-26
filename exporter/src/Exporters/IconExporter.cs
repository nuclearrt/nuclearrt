using System.Drawing;
using System.Drawing.Text;
using System.Text;
using CTFAK.CCN.Chunks.Banks;
using CTFAK.Core.CCN.Chunks.Banks.ImageBank;
using CTFAK.Utils;

public class IconExporter : BaseExporter
{
	public IconExporter(Exporter exporter) : base(exporter) { }

	public override void Export()
	{
		WriteRawIcons();
		WriteWindowsIcons();
		WriteMacIcons();
		WriteSwitchIcon();
	}

	void WriteRawIcons()
	{
		List<FusionImage> possibleIcons = [];

		foreach (var iconHandle in MfaData.IconImages)
		{
			if (!MfaData.Icons.Items.ContainsKey(iconHandle))
			{
				Logger.Log("Icon handle not found: " + iconHandle);
				continue;
			}

			possibleIcons.Add(MfaData.Icons.Items[iconHandle]);
		}

		possibleIcons.Sort((x, y) => x.imageData.Length.CompareTo(y.imageData.Length));

		Directory.CreateDirectory(Path.Combine(OutputPath.FullName, "icons", "raw"));

		foreach (var file in Directory.GetFiles(Path.Combine(OutputPath.FullName, "icons", "raw")))
		{
			File.Delete(file);
		}

		foreach (var icon in possibleIcons)
		{
			icon.bitmap.Save(Path.Combine(OutputPath.FullName, "icons", "raw", $"{icon.Width}.png"));
		}
	}

	void WriteWindowsIcons()
	{
		var rawIcons = Directory.GetFiles(Path.Combine(OutputPath.FullName, "icons", "raw")).Where(f => f.EndsWith(".png")).ToList();

		List<Bitmap> bitmaps = [];
		foreach (var rawIcon in rawIcons)
		{
			bitmaps.Add((Bitmap)Bitmap.FromFile(rawIcon));
		}

		using (var stream = new FileStream(Path.Combine(OutputPath.FullName, "icons", "windows.ico"), FileMode.Create))
		{
			IconFactory.SavePngsAsIcon(bitmaps, stream);
		}
	}

	void WriteMacIcons()
	{
		List<(string, byte[])> rawIcons = [.. Directory.GetFiles(Path.Combine(OutputPath.FullName, "icons", "raw")).Where(f => f.EndsWith(".png")).Select(f => (Path.GetFileName(f), File.ReadAllBytes(f)))];

		Directory.CreateDirectory(Path.Combine(OutputPath.FullName, "copy", "mac"));
		BinaryWriter writer = new(File.Open(Path.Combine(OutputPath.FullName, "copy", "mac", "AppIcon.icns"), FileMode.Create));

		writer.Write("icns".ToCharArray());
		writer.Write(0);

		foreach (var rawIcon in rawIcons)
		{
			string? iconType = rawIcon.Item1 switch
			{
				"128.png" => "ic07",
				"256.png" => "ic08",
				_ => null
			};

			if (iconType == null)
			{
				continue;
			}

			writer.Write(Encoding.ASCII.GetBytes(iconType));
			writer.Write(BitConverter.GetBytes(rawIcon.Item2.Length + 8).Reverse().ToArray());
			writer.Write(rawIcon.Item2);
		}

		writer.BaseStream.Seek(4, SeekOrigin.Begin);

		//write in big endian
		writer.Write(BitConverter.GetBytes((int)writer.BaseStream.Length).Reverse().ToArray());
	}

	void WriteSwitchIcon()
	{
		string rawDir = Path.Combine(OutputPath.FullName, "icons", "raw");
		string source = Path.Combine(rawDir, "256.png");
		if (!File.Exists(source))
		{
			// fallback to the largest icon
			source = Directory.GetFiles(rawDir, "*.png").OrderByDescending(f => int.TryParse(Path.GetFileNameWithoutExtension(f), out int w) ? w : 0).FirstOrDefault() ?? "";
			if (string.IsNullOrWhiteSpace(source)) return;
		}

		using Bitmap icon = (Bitmap)Bitmap.FromFile(source);
		using Bitmap jpg = new(256, 256);
		using (Graphics g = Graphics.FromImage(jpg))
		{
			g.Clear(Color.Black);
			g.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.HighQualityBicubic;
			g.DrawImage(icon, 0, 0, 256, 256);
		}
		jpg.Save(Path.Combine(OutputPath.FullName, "icons", "switch.jpg"), System.Drawing.Imaging.ImageFormat.Jpeg);
	}
}
