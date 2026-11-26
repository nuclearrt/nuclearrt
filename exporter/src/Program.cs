using Avalonia;
using CTFAK.EXE;
using CTFAK.Utils;

namespace NuclearRTExporter
{
	class Program
	{
		public static string[] StartupArgs { get; private set; } = Array.Empty<string>();

		// Initialization code. Don't use any Avalonia, third-party APIs or any
		// STA threading model dependencies here, as they may not be initialized yet.
		public static void Main(string[] args)
		{
			StartupArgs = args;
			BuildAvaloniaApp().StartWithClassicDesktopLifetime(args);
		}

		// Avalonia configuration, don't remove; also used by visual designer.
		public static AppBuilder BuildAvaloniaApp()
				=> AppBuilder.Configure<App>()
						.UsePlatformDetect()
						.LogToTrace();

		public static ExportSettings? ParseArguments()
		{
			try
			{
				// Check if we have enough arguments
				if (StartupArgs.Length < 3)
				{
					return null;
				}

				BuildType buildType = Enum.Parse<BuildType>(StartupArgs[0]);
				string ccnPath = StartupArgs[1];
				string outputPath = StartupArgs[2];

				if (string.IsNullOrEmpty(ccnPath) || string.IsNullOrEmpty(outputPath))
				{
					return null;
				}

				return new ExportSettings
				{
					BuildType = buildType,
					CcnPath = ccnPath,
					OutputPath = outputPath,
				};
			}
			catch (Exception ex)
			{
				Console.WriteLine($"Error parsing arguments: {ex.Message}");
				return null;
			}
		}
	}

	public class ExportSettings
	{
		public BuildType BuildType { get; set; } = BuildType.SourceCode;
		public string CcnPath { get; set; } = string.Empty;
		public string OutputPath { get; set; } = string.Empty;
	}

	public class NuclearRTExporter
	{
		private Action<string>? logAction;

		public NuclearRTExporter(Action<string>? logger = null)
		{
			logAction = logger;
		}

		public bool Run(ExportSettings settings)
		{
			try
			{
				// Validate inputs
				if (string.IsNullOrEmpty(settings.CcnPath))
				{
					Log("CCN path cannot be empty!");
					return false;
				}

				if (!File.Exists(settings.CcnPath))
				{
					Log("CCN file does not exist!");
					return false;
				}

				if (string.IsNullOrEmpty(settings.OutputPath))
				{
					Log("Output path cannot be empty!");
					return false;
				}

				DirectoryInfo outputDir = new DirectoryInfo(settings.OutputPath); // get the directory name because the output path is a file
				outputDir = outputDir.Parent;

				//find the runtime base directory by going up the current directory until we find the runtime directory somewhere in the folder structure
				DirectoryInfo runtimeBaseDir = new DirectoryInfo(Directory.GetCurrentDirectory());
				runtimeBaseDir = runtimeBaseDir.Parent;
				foreach (DirectoryInfo subDir in runtimeBaseDir.GetDirectories())
				{
					if (subDir.Name.Equals("runtime", StringComparison.OrdinalIgnoreCase))
					{
						runtimeBaseDir = subDir;
						break;
					}
				}

				if (!runtimeBaseDir.Name.Equals("runtime", StringComparison.OrdinalIgnoreCase))
				{
					Log("Runtime base directory not found!");
					return false;
				}
				Log("Runtime base directory: " + runtimeBaseDir.FullName);

				Log("Reading .ccn file...");
				CCNFileReader ccnReader = new CCNFileReader();
				ccnReader.LoadGame(settings.CcnPath);

				//Read MFA file
				Log("Reading .mfa file...");
				MFAFileReader mfaReader = new MFAFileReader();
				mfaReader.LoadGame(ccnReader.game.editorFilename);

				ProjectConfig config = ProjectConfig.Load(ccnReader.game.editorFilename, ccnReader.getGameData(), mfaReader.mfa);

				// Export and rewrite game data in the runtime code
				Log("Exporting with C++ runtime...");
				Exporter exporter = new Exporter(ccnReader, mfaReader, config, runtimeBaseDir, outputDir);
				exporter.Export();

				// Extract the resources from the game data and write them to a pak file
				Log("Extracting resources...");
				PakBuilder pakBuilder = new PakBuilder();
				pakBuilder.Build(ccnReader, mfaReader, outputDir);

				// Compile the project
				Log("Compiling project...");
				Compiler compiler = new Compiler();
				compiler.Compile(settings.BuildType, outputDir);

				Log("Export completed successfully!");
				return true;
			}
			catch (Exception ex)
			{
				Log($"Export failed: {ex.Message}");
				Log(ex.StackTrace);
				return false;
			}
		}

		// Log a message to the console or UI
		public void Log(string message)
		{
			CTFAK.Utils.Logger.Log(message);
		}
	}
}
