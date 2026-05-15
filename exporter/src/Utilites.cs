using CTFAK.CCN.Chunks.Frame;
using CTFAK.FileReaders;

public static class Utilities
{
	public static string GetObjectName(IFileReader reader, int objectInfo)
	{
		return reader.getGameData().frameitems[objectInfo].name;
	}

	public static Quailifer? FindFrameQualifier(int frameIndex, int mfaObjectInfoHandle, int systemQualifier)
	{
		Quailifer? quailifer = Exporter.Instance.GameData.Frames[frameIndex].events.QualifiersList.Find(q => q.ObjectInfo == mfaObjectInfoHandle);
		if (quailifer != null) return quailifer;
		
		if (systemQualifier != 0)
		{
			int packedOi = 32768 + systemQualifier;
			return Exporter.Instance.GameData.Frames[frameIndex].events.QualifiersList.Find(q => q.ObjectInfo == packedOi);
		}

		return null;
	}

	public static string GetQualifierName(int groupIndex, int qualifierType)
	{
		string qualifierName = "Group_" + (groupIndex) switch
		{
			0 => "Player",
			1 => "Good",
			2 => "Neutral",
			3 => "Bad",
			4 => "Enemies",
			5 => "Friends",
			6 => "Bullets",
			7 => "Arms",
			8 => "Bonus",
			9 => "Collectables",
			10 => "Traps",
			11 => "Doors",
			12 => "Keys",
			13 => "Texts",
			14 => "0",
			15 => "1",
			16 => "2",
			17 => "3",
			18 => "4",
			19 => "5",
			20 => "6",
			21 => "7",
			22 => "8",
			23 => "9",
			24 => "Parents",
			25 => "Children",
			26 => "Data",
			27 => "Timed",
			28 => "Engine",
			29 => "Areas",
			30 => "Reference_Points",
			31 => "Radar_Enemies",
			32 => "Radar_Friends",
			33 => "Radar_Neutrals",
			34 => "Music",
			35 => "Sound",
			36 => "Waveform",
			37 => "Background_Scenery",
			38 => "Foreground_Scenery",
			39 => "Decorations",
			40 => "Water",
			41 => "Clouds",
			42 => "Empty",
			43 => "Fog",
			44 => "Flowers",
			45 => "Animals",
			46 => "Bosses",
			47 => "NPC",
			48 => "Vehicles",
			49 => "Rockets",
			50 => "Balls",
			51 => "Bombs",
			52 => "Explosions",
			53 => "Particles",
			54 => "Clothes",
			55 => "Glow",
			56 => "Arrows",
			57 => "Buttons",
			58 => "Cursors",
			59 => "Drawing_Tools",
			60 => "Indicator",
			61 => "Shapes",
			62 => "Shields",
			63 => "Shifting_Blocks",
			64 => "Magnets",
			65 => "Negative_Matter",
			66 => "Neutral_Matter",
			67 => "Positive_Matter",
			68 => "Breakable",
			69 => "Dissolving",
			70 => "Dialogue",
			71 => "HUD",
			72 => "Inventory",
			73 => "Inventory_Item",
			74 => "Interface",
			75 => "Movable",
			76 => "Perspective",
			77 => "Calculation_Objects",
			78 => "Invisible",
			79 => "Masks",
			80 => "Obstacles",
			81 => "Value Holder",
			82 => "Helpful",
			83 => "Powerups",
			84 => "Targets",
			85 => "Trapdoors",
			86 => "Dangers",
			87 => "Forbidden",
			88 => "Physical_objects",
			89 => "3D_Objects",
			90 => "Generic_1",
			91 => "Generic_2",
			92 => "Generic_3",
			93 => "Generic_4",
			94 => "Generic_5",
			95 => "Generic_6",
			96 => "Generic_7",
			97 => "Generic_8",
			98 => "Generic_9",
			99 => "Generic_10",
			_ => string.Empty
		};

		qualifierName += "." + qualifierType switch
		{
			2 => "Sprite",
			3 => "Text",
			4 => "Question",
			5 => "Score",
			6 => "Lives",
			7 => "Counter",
			_ => string.Empty
		};

		return qualifierName;
	}
}
