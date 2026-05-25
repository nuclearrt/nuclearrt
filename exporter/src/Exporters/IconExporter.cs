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
}
