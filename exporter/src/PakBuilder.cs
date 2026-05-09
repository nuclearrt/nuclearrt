using System.Drawing.Text;
using CTFAK.EXE;
using CTFAK.FileReaders;
using CTFAK.Memory;
using CTFAK.Utils;

public class PakBuilder
{
	private List<PakEntry> entries = [];

	public void Build(CCNFileReader ccnReader, MFAFileReader mfaReader, DirectoryInfo outputPath)
	{
		//read the game data
		var gameData = ccnReader.getGameData();
		var mfaData = mfaReader.getMfaData();

		//images
		foreach (var image in gameData.Images.Items.Values)
		{
			var entry = new PakEntry { Path = $"images/{image.Handle}.png" };
			using var imageStream = new MemoryStream();
			image.bitmap.Save(imageStream, System.Drawing.Imaging.ImageFormat.Png);
			entry.Size = (uint)imageStream.Length;
			entry.Data = imageStream.ToArray();
			entries.Add(entry);
		}

		//collision masks
		var collisionMasks = CollisionMaskBuilder.BuildCollisionMask(gameData);
		foreach (var mask in collisionMasks)
		{
			var entry = new PakEntry { Path = $"images/masks/{mask.Handle}.bin" };
			entry.Size = (uint)mask.Data.Length;
			entry.Data = mask.Data;
			entries.Add(entry);
		}

		//sounds
		foreach (var sound in mfaData.Sounds.Items)
		{
			var entry = new PakEntry { Path = $"sounds/{sound.Handle}.{GetAudioExtension(sound.Data[0..4])}" };
			entry.Size = (uint)sound.Data.Length;
			entry.Data = sound.Data;
			entries.Add(entry);
		}

		//fonts
		Dictionary<string, List<string>> fontNames = new Dictionary<string, List<string>>(); // font family name, font file names
		var fontsFolder = new DirectoryInfo(Environment.GetFolderPath(Environment.SpecialFolder.Fonts));
		FileInfo[] fontFiles = fontsFolder.GetFiles();
		foreach (var fontFile in fontFiles)
		{
			//go through each file and try to find one with the same name as a Application Font
			List<string> appFontNames = [];
			using (PrivateFontCollection fontCollection = new PrivateFontCollection())
			{
				fontCollection.AddFontFile(fontFile.FullName);
				foreach (var font in fontCollection.Families)
				{
					if (!fontNames.TryGetValue(font.Name, out List<string> fontFileNames))
					{
						fontFileNames = [];
						fontNames.Add(font.Name, fontFileNames);
					}

					fontFileNames.Add(fontFile.Name);
				}
			}
		}

		foreach (var font in gameData.Fonts.Items)
		{
			if (fontNames.TryGetValue(font.Value.FaceName.Replace("\0", string.Empty), out List<string> fontFileNames))
			{
				foreach (var fontFileName in fontFileNames)
				{
					var entry = new PakEntry { Path = $"fonts/{fontFileName}" };
					entry.Data = File.ReadAllBytes(Path.Combine(fontsFolder.FullName, fontFileName));
					entry.Size = (uint)entry.Data.Length;
					entries.Add(entry);
				}
			}
		}

		//standard shaders
		var shaderFolder = new DirectoryInfo(Path.Combine(outputPath.FullName, "shaders", "standard"));
		if (shaderFolder.Exists)
		{
			foreach (var shaderFile in shaderFolder.GetFiles("*", SearchOption.AllDirectories))
			{
				var relativePath = Path.GetRelativePath(shaderFolder.FullName, shaderFile.FullName).Replace('\\', '/');
				var entry = new PakEntry { Path = $"shaders/standard/{relativePath}" };
				entry.Data = File.ReadAllBytes(shaderFile.FullName);
				entry.Size = (uint)entry.Data.Length;
				entries.Add(entry);
			}
		}

		//thirdparty shaders
		var thirdPartyShaders = EffectBankExporter.thirdPartyShaders;
		foreach (var shader in thirdPartyShaders)
		{
			var entry = new PakEntry { Path = $"shaders/thirdparty/{shader.Key}.frag" };
			entry.Data = File.ReadAllBytes(shader.Value.FullName);
			entry.Size = (uint)entry.Data.Length;
			entries.Add(entry);
		}
		Directory.Delete(Path.Combine(outputPath.FullName, "shaders"), true);

		//clear any duplicates
		entries = entries.DistinctBy(e => e.Path).ToList();

		//calculate offsets
		uint dataOffset = 12; // header size
		foreach (var entry in entries)
		{
			entry.Offset = dataOffset;
			dataOffset += entry.Size;
		}

		//create quake pak file
		using var pak = File.Create(outputPath + "/copy/assets.pak");
		using var writer = new ByteWriter(pak);

		//write header
		writer.WriteAscii("PACK"); // magic
		writer.WriteUInt32(dataOffset); // directory offset
		writer.WriteUInt32((uint)(entries.Count * 64)); // directory size

		//write file data
		foreach (var entry in entries)
		{
			pak.Write(entry.Data, 0, (int)entry.Size);
		}

		//write directory
		foreach (var entry in entries)
		{
			//pad filename to 56 bytes
			byte[] fileNameBytes = System.Text.Encoding.ASCII.GetBytes(entry.Path);
			byte[] paddedFileName = new byte[56];
			Array.Copy(fileNameBytes, paddedFileName, Math.Min(fileNameBytes.Length, 56));
			writer.WriteBytes(paddedFileName);

			writer.WriteUInt32(entry.Offset);
			writer.WriteUInt32(entry.Size);
		}

		pak.Flush();
		pak.Close();
	}

	public static string GetAudioExtension(byte[] magic)
	{
		if (magic[0] == 0xFF && magic[1] == 0xFB ||
			magic[0] == 0xFF && magic[1] == 0xF3 ||
			magic[0] == 0xFF && magic[1] == 0xF2 ||
			magic[0] == 0x49 && magic[1] == 0x44 && magic[2] == 0x33
		)
			return "mp3";

		if (magic[0] == 0x52 && magic[1] == 0x49 && magic[2] == 0x46 && magic[3] == 0x46)
			return "wav";

		if (magic[0] == 0x4F && magic[1] == 0x67 && magic[2] == 0x67 && magic[3] == 0x53)
			return "ogg";

		return "wav";
	}
}

public class PakEntry
{
	public string Path { get; set; }
	public uint Offset { get; set; }
	public uint Size { get; set; }
	public byte[] Data { get; set; } = [];
}
