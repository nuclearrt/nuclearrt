using System.Diagnostics;

public class Compiler
{
        private static readonly string CMakePath = @"C:\Program Files\CMake\bin\cmake.exe";

	public void Compile(BuildType buildType, DirectoryInfo outputPath)
	{
		if (buildType == BuildType.SourceCode) return;

		if (buildType == BuildType.WindowsDebug || buildType == BuildType.WindowsRelease)
		{
			//make 'build/windows' directory
			DirectoryInfo buildDir = new DirectoryInfo(Path.Combine(outputPath.FullName, "build", "windows"));
			buildDir.Create();

			CTFAK.Utils.Logger.Log($"Running CMake to generate the project files...");
			ProcessStartInfo cmakeProjectInfo = new ProcessStartInfo(CMakePath, "../..");
			cmakeProjectInfo.WorkingDirectory = Path.Combine(outputPath.FullName, "build", "windows");
			cmakeProjectInfo.CreateNoWindow = true;
			cmakeProjectInfo.UseShellExecute = false;
			cmakeProjectInfo.RedirectStandardOutput = true;
			cmakeProjectInfo.RedirectStandardError = true;
			Process cmakeProcess = Process.Start(cmakeProjectInfo);

			cmakeProcess.OutputDataReceived += (sender, e) =>
			{
				CTFAK.Utils.Logger.Log(e.Data);
			};

			cmakeProcess.ErrorDataReceived += (sender, e) =>
			{
				CTFAK.Utils.Logger.Log(e.Data);
			};

			cmakeProcess.BeginOutputReadLine();
			cmakeProcess.BeginErrorReadLine();
			cmakeProcess.WaitForExit();

			if (cmakeProcess.ExitCode != 0)
			{
				CTFAK.Utils.Logger.Log($"CMake failed with exit code {cmakeProcess.ExitCode}");
				throw new Exception("CMake failed to generate the project files");
			}

			CTFAK.Utils.Logger.Log($"Building...");
			ProcessStartInfo buildInfo = new ProcessStartInfo(CMakePath, "--build . --config " + (buildType == BuildType.WindowsDebug ? "Debug" : "Release"));
			buildInfo.WorkingDirectory = Path.Combine(outputPath.FullName, "build", "windows");
			buildInfo.CreateNoWindow = true;
			buildInfo.UseShellExecute = false;
			buildInfo.RedirectStandardOutput = true;
			buildInfo.RedirectStandardError = true;
			Process buildProcess = Process.Start(buildInfo);

			buildProcess.OutputDataReceived += (sender, e) =>
			{
				CTFAK.Utils.Logger.Log(e.Data);
			};

			buildProcess.ErrorDataReceived += (sender, e) =>
			{
				CTFAK.Utils.Logger.Log(e.Data);
			};

			buildProcess.BeginOutputReadLine();
			buildProcess.BeginErrorReadLine();
			buildProcess.WaitForExit();

			if (buildProcess.ExitCode != 0)
			{
				CTFAK.Utils.Logger.Log($"Build failed with exit code {buildProcess.ExitCode}");
				throw new Exception("Build failed");
			}
		}
		else if (buildType == BuildType.Web)
		{
			DirectoryInfo buildDir = new DirectoryInfo(Path.Combine(outputPath.FullName, "build", "web"));
			buildDir.Create();

			CTFAK.Utils.Logger.Log($"Running CMake to generate the project files...");
			ProcessStartInfo cmakeProjectInfo = new ProcessStartInfo(
				"cmd.exe",
				"/c emcmake cmake ../.. -DCMAKE_BUILD_TYPE=Release -G Ninja"
			);
			cmakeProjectInfo.WorkingDirectory = Path.Combine(outputPath.FullName, "build", "web");
			cmakeProjectInfo.CreateNoWindow = true;
			cmakeProjectInfo.UseShellExecute = false;
			cmakeProjectInfo.RedirectStandardOutput = true;
			cmakeProjectInfo.RedirectStandardError = true;
			Process cmakeProcess = Process.Start(cmakeProjectInfo);

			cmakeProcess.OutputDataReceived += (sender, e) =>
			{
				CTFAK.Utils.Logger.Log(e.Data);
			};

			cmakeProcess.ErrorDataReceived += (sender, e) =>
			{
				CTFAK.Utils.Logger.Log(e.Data);
			};

			cmakeProcess.BeginOutputReadLine();
			cmakeProcess.BeginErrorReadLine();
			cmakeProcess.WaitForExit();

			if (cmakeProcess.ExitCode != 0)
			{
				CTFAK.Utils.Logger.Log($"CMake failed with exit code {cmakeProcess.ExitCode}");
				throw new Exception("CMake failed to generate the project files");
			}

			CTFAK.Utils.Logger.Log($"Building...");
			ProcessStartInfo buildInfo = new ProcessStartInfo(
				"cmd.exe",
				"/c cmake --build . --config Release"
			);
			buildInfo.WorkingDirectory = Path.Combine(outputPath.FullName, "build", "web");
			buildInfo.CreateNoWindow = true;
			buildInfo.UseShellExecute = false;
			buildInfo.RedirectStandardOutput = true;
			buildInfo.RedirectStandardError = true;
			Process buildProcess = Process.Start(buildInfo);

			buildProcess.OutputDataReceived += (sender, e) =>
			{
				CTFAK.Utils.Logger.Log(e.Data);
			};

			buildProcess.ErrorDataReceived += (sender, e) =>
			{
				CTFAK.Utils.Logger.Log(e.Data);
			};

			buildProcess.BeginOutputReadLine();
			buildProcess.BeginErrorReadLine();
			buildProcess.WaitForExit();

			if (buildProcess.ExitCode != 0)
			{
				CTFAK.Utils.Logger.Log($"Build failed with exit code {buildProcess.ExitCode}");
				throw new Exception("Build failed");
			}
		}
	}
}
