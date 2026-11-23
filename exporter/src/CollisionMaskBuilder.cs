using CTFAK.CCN;
using CTFAK.Core.CCN.Chunks.Banks.ImageBank;
using CTFAK.FileReaders;
using System.Drawing;
using System.Linq;
using static CollisionMaskBuilder;

public class CollisionMaskBuilder
{
	public struct CollisionMask
	{
		public int Handle;
		public int Width;
		public int Height;
		public byte[] Data;
	}

	private const int ALPHA_THRESHOLD = 128; //TODO: verify if this is accurate

	public static List<CollisionMask> BuildCollisionMask(GameData gameData)
	{
		var collisionMasks = new List<CollisionMask>();

		foreach (var image in gameData.Images.Items.Values)
		{
			var collisionMask = new CollisionMask { Handle = image.Handle, Width = image.Width, Height = image.Height };
			Bitmap bitmap = image.bitmap;
			int rowStride = (image.Width + 7) / 8;
			var data = new byte[rowStride * image.Height];
			
			for (int y = 0; y < image.Height; y++)
			{
				for (int x = 0; x < image.Width; x += 8)
				{
					byte mask = 0;
					for (int i = 0; i < 8; i++)
					{
						if (x + i < image.Width)
						{
							bool isSolid = bitmap.GetPixel(x + i, y).A > ALPHA_THRESHOLD;
							if (isSolid)
							{
								mask |= (byte)(1 << (7 - i));
							}
						}
					}
					data[(y * rowStride) + (x / 8)] = mask;
				}
			}

			collisionMask.Data = data;
			collisionMasks.Add(collisionMask);
		}

		return collisionMasks;
	}

	public static CollisionMask GenerateCollisionMask(FusionImage image)
	{
		var collisionMask = new CollisionMask { Handle = image.Handle, Width = image.Width, Height = image.Height };
		Bitmap bitmap = image.bitmap;
		int rowStride = (image.Width + 7) / 8;
		var data = new byte[rowStride * image.Height];


		for (int y = 0; y < image.Height; y++)
		{
			for (int x = 0; x < image.Width; x += 8)
			{
				byte mask = 0;
				for (int i = 0; i < 8; i++)
				{
					if (x + i < image.Width)
					{
						bool isSolid = bitmap.GetPixel(x + i, y).A > ALPHA_THRESHOLD;
						if (isSolid)
						{
							mask |= (byte)(1 << (7 - i));
						}
					}
				}
				data[(y * rowStride) + (x / 8)] = mask;
			}
		}

		collisionMask.Data = data;
		return collisionMask;
	}

	// NOTE: imageData is expected to be in Format32bppArgb format
	public static CollisionMask GenerateCollisionMask(FusionImage image, byte[] imageData)
	{
		var collisionMask = new CollisionMask { Handle = image.Handle, Width = image.Width, Height = image.Height };
		int rowStride = (image.Width + 7) / 8;
		var data = new byte[rowStride * image.Height];


		for (int y = 0; y < image.Height; y++)
		{
			for (int x = 0; x < image.Width; x += 8)
			{
				byte mask = 0;
				for (int i = 0; i < 8; i++)
				{
					if (x + i < image.Width)
					{
						bool isSolid = imageData[((y * image.Width + (x + i)) * 4) + 3] > ALPHA_THRESHOLD;
						if (isSolid)
						{
							mask |= (byte)(1 << (7 - i));
						}
					}
				}
				data[(y * rowStride) + (x / 8)] = mask;
			}
		}

		collisionMask.Data = data;
		return collisionMask;
	}

}
