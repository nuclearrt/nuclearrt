using System.Drawing;
using System.Drawing.Imaging;
using System.Drawing.Text;
using System.Runtime.InteropServices;
using CTFAK.EXE;
using CTFAK.FileReaders;
using CTFAK.Memory;
using CTFAK.Utils;
using System.Threading.Tasks;

using static CollisionMaskBuilder;

public class PakBuilder
{
	public void Build(CCNFileReader ccnReader, MFAFileReader mfaReader, DirectoryInfo outputPath)
	{
		BuildMainPak(ccnReader, mfaReader, outputPath);
		BuildShaderPak(outputPath);
	}

	void BuildMainPak(CCNFileReader ccnReader, MFAFileReader mfaReader, DirectoryInfo outputPath)
	{
		var gameData = ccnReader.getGameData();
		var mfaData = mfaReader.getMfaData();

		string curHash = GetMainBankHash(ccnReader, mfaReader);

		string oldHash = "";
		var hashFile = new FileInfo(Path.Combine(outputPath.FullName, ".assets.hash"));
		if (hashFile.Exists)
		{
			oldHash = File.ReadAllText(hashFile.FullName);
		}

		if (curHash == oldHash)
		{
			Logger.Log("Assets have not changed, skipping main pak rebuild.");
			return;
		}
		else
		{
			Logger.Log("Assets have changed, rebuilding main pak.");
		}

		PakFile mainPak = new PakFile();

		var imageEntries = new PakEntry[(gameData.Images.Items.Count)];
		var MaskEntries = new PakEntry[gameData.Images.Items.Count];

		var options = new ParallelOptions
		{
		    MaxDegreeOfParallelism = Math.Max(1, Environment.ProcessorCount - 1)
		};

		//images & masks
		Parallel.ForEach(gameData.Images.Items.Values, options, (image, state, index) =>
		{
			// generate an image entry
			var imageEntry = new PakEntry { Path = $"images/{image.Handle}.rgba" };

			Bitmap imagebitmap = image.bitmap;
			byte[] imageRawData = new byte[imagebitmap.Width * imagebitmap.Height * 4];
			BitmapData bitmapData = imagebitmap.LockBits(new Rectangle(0, 0, imagebitmap.Width, imagebitmap.Height), ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);
			Marshal.Copy(bitmapData.Scan0, imageRawData, 0, imageRawData.Length);
			imagebitmap.UnlockBits(bitmapData);

			MemoryStream imageStream = WriteCompressedBitMap(imageRawData, image.Flags["Alpha"] || image.Flags["RGBA"]);
			imageEntry.Size = (uint)imageStream.Length;
			imageEntry.Data = imageStream.ToArray();
			imageStream.Close();
			imageEntries[index] = imageEntry;


			// generate an image entry
			CollisionMask collisionMask = GenerateCollisionMask(image, imageRawData);

			var maskEntry = new PakEntry { Path = $"images/masks/{collisionMask.Handle}.bin" };
			MemoryStream maskStream = WriteCompressedMask(collisionMask);
			maskEntry.Size = (uint)maskStream.Length;
			maskEntry.Data = maskStream.ToArray();
			maskStream.Close();
			MaskEntries[index] = maskEntry;
		});

		foreach (var entry in imageEntries)
		{
			mainPak.AddEntry(entry);
		}

		foreach (var entry in MaskEntries)
		{
			mainPak.AddEntry(entry);
		}

		//sounds
		foreach (var sound in gameData.Sounds.Items)
		{
			var entry = new PakEntry { Path = $"sounds/{sound.Handle-1}.{GetAudioExtension(sound.Data[0..4])}" };
			entry.Size = (uint)sound.Data.Length;
			entry.Data = sound.Data;
			mainPak.AddEntry(entry);
		}

		//fonts
		Dictionary<string, List<string>> fontNames = new Dictionary<string, List<string>>(); // font family name, font file names
		var fontsFolders = new DirectoryInfo[] {
			new(Environment.GetFolderPath(Environment.SpecialFolder.Fonts)),
			new(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Microsoft", "Windows", "Fonts"))
		};
		foreach (var fontsFolder in fontsFolders)
		{
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

						fontFileNames.Add(fontFile.FullName);
					}
				}
			}
		}

		foreach (var font in gameData.Fonts.Items)
		{
			if (fontNames.TryGetValue(font.Value.FaceName.Replace("\0", string.Empty), out List<string> fontFileNames))
			{
				foreach (var fontFileName in fontFileNames)
				{
					var entry = new PakEntry { Path = $"fonts/{Path.GetFileName(fontFileName)}" };
					entry.Data = File.ReadAllBytes(fontFileName);
					entry.Size = (uint)entry.Data.Length;
					mainPak.AddEntry(entry);
				}
			}
		}

		//write out hash
		File.WriteAllText(Path.Combine(outputPath.FullName, ".assets.hash"), GetMainBankHash(ccnReader, mfaReader));

		mainPak.Save(Path.Combine(outputPath.FullName, "copy", "all", "assets.pak"));
	}

	public string GetMainBankHash(CCNFileReader ccnReader, MFAFileReader mfaReader)
	{
		var gameData = ccnReader.getGameData();
		string hash = "";

		hash += gameData.Images.bankHash;
		hash += gameData.Sounds.bankHash;
		hash += gameData.Fonts.bankHash;

		return hash;
	}

	void BuildShaderPak(DirectoryInfo outputPath)
	{
		Dictionary<Tuple<string, List<string>>, PakFile> shaderVersions = new Dictionary<Tuple<string, List<string>>, PakFile>() {
			{ Tuple.Create("gles300", new List<string> { "web", "mobile" }), new PakFile() },
			{ Tuple.Create("gl330", new List<string> { "desktop", "wiiu" }), new PakFile() }
		};
		var baseShaderFolder = new DirectoryInfo(Path.Combine(outputPath.FullName, "shaders"));
		if (baseShaderFolder.Exists)
		{
			foreach (var shaderVersion in shaderVersions.Keys)
			{
				var shaderFolder = new DirectoryInfo(Path.Combine(outputPath.FullName, "shaders", shaderVersion.Item1, "standard"));
				if (!shaderFolder.Exists) continue;
				foreach (var shaderFile in shaderFolder.GetFiles("*", SearchOption.AllDirectories))
				{
					var relativePath = Path.GetRelativePath(shaderFolder.FullName, shaderFile.FullName).Replace('\\', '/');
					var entry = new PakEntry { Path = $"shaders/standard/{relativePath}" };
					entry.Data = File.ReadAllBytes(shaderFile.FullName);
					entry.Size = (uint)entry.Data.Length;
					shaderVersions[shaderVersion].AddEntry(entry);
				}
			}
		}

		foreach (var shaderVersion in shaderVersions.Keys)
		{
			foreach (var hash in EffectBankExporter.thirdPartyShaderHashes)
			{
				var fragFile = EffectBankExporter.GetThirdPartyFrag(outputPath, shaderVersion.Item1, hash);
				byte[] data;
				if (fragFile != null && fragFile.Exists)
					data = File.ReadAllBytes(fragFile.FullName);
				else if (EffectBankExporter.translatedShaders.TryGetValue(hash, out var generated))
					data = System.Text.Encoding.UTF8.GetBytes(shaderVersion.Item1 == "gles300" ? generated.Gles300 : generated.Gl330);
				else
					continue;

				var entry = new PakEntry { Path = $"shaders/thirdparty/{hash}.frag" };
				entry.Data = data;
				entry.Size = (uint)entry.Data.Length;
				shaderVersions[shaderVersion].AddEntry(entry);
			}
		}

		Directory.Delete(Path.Combine(outputPath.FullName, "shaders"), true);

		foreach (var kv in shaderVersions)
		{
			foreach (var target in kv.Key.Item2)
			{
				kv.Value.Save(Path.Combine(outputPath.FullName, "copy", target, $"shaders-{kv.Key.Item1}.pak"));
			}
		}
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

	public static MemoryStream WriteCompressedImage(Bitmap image)
	{
		byte[] uncomp = new byte[image.Width * image.Height * 4];
		BitmapData bitmapData = image.LockBits(new Rectangle(0, 0, image.Width, image.Height), ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);
		Marshal.Copy(bitmapData.Scan0, uncomp, 0, uncomp.Length);
		image.UnlockBits(bitmapData);

		// swap BGRA to RGBA
		for (int i = 0; i < uncomp.Length; i += 4)
		{
			(uncomp[i], uncomp[i + 2]) = (uncomp[i + 2], uncomp[i]);
		}

		byte[] comp = new byte[K4os.Compression.LZ4.LZ4Codec.MaximumOutputSize(uncomp.Length)];
		int encodedLength = K4os.Compression.LZ4.LZ4Codec.Encode(
			uncomp, 0, uncomp.Length,
			comp, 0, comp.Length,
			K4os.Compression.LZ4.LZ4Level.L09_HC);

		return new MemoryStream(comp, 0, encodedLength);
	}

	// expects Format32bppArgb bitmaps!
	public static MemoryStream WriteCompressedBitMap(byte[] uncomp, bool hasAlphaChannel = true)
	{
		// swap BGRA to RGBA
		for (int i = 0; i < uncomp.Length; i += 4)
		{
			(uncomp[i], uncomp[i + 2]) = (uncomp[i + 2], uncomp[i]);
			// images masked by a transparent colour become black
			if (!hasAlphaChannel && uncomp[i + 3] == 0)
				uncomp[i] = uncomp[i + 1] = uncomp[i + 2] = 0;
		}

		byte[] comp = new byte[K4os.Compression.LZ4.LZ4Codec.MaximumOutputSize(uncomp.Length)];
		int encodedLength = K4os.Compression.LZ4.LZ4Codec.Encode(
			uncomp, 0, uncomp.Length,
			comp, 0, comp.Length,
			K4os.Compression.LZ4.LZ4Level.L09_HC);

		return new MemoryStream(comp, 0, encodedLength);
	}

	public static MemoryStream WriteCompressedMask(CollisionMask mask)
	{
		byte[] uncomp = mask.Data;

		byte[] comp = new byte[K4os.Compression.LZ4.LZ4Codec.MaximumOutputSize(uncomp.Length)];
		int compressedSize = K4os.Compression.LZ4.LZ4Codec.Encode(
			uncomp,
			0,
			uncomp.Length,
			comp,
			0,
			comp.Length,
			K4os.Compression.LZ4.LZ4Level.L09_HC
		);

		var ms = new MemoryStream();
		var bw = new BinaryWriter(ms);

		bw.Write((ushort)mask.Width);
		bw.Write((ushort)mask.Height);
		bw.Write(mask.Data.Length);
		bw.Write(comp, 0, compressedSize);

		bw.Flush();
		ms.Position = 0;

		return ms;
	}
}

public class PakEntry
{
	public string Path { get; set; }
	public uint Offset { get; set; }
	public uint Size { get; set; }
	public byte[] Data { get; set; } = [];
}

public class PakFile
{
	private List<PakEntry> entries = [];

	public void AddEntry(PakEntry entry)
	{
		entries.Add(entry);
	}

	public void Save(string path)
	{
		Directory.CreateDirectory(Path.GetDirectoryName(path));

		entries = entries.DistinctBy(e => e.Path).ToList();

		//calculate offsets
		uint dataOffset = 12; // header size
		foreach (var entry in entries)
		{
			entry.Offset = dataOffset;
			dataOffset += entry.Size;
		}

		//create quake pak file
		using var pak = File.Create(path);
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
}
