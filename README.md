# NuclearRT

NuclearRT is a fast, open-source, cross-platform C++ runtime for Clickteam Fusion 2.5.

![NuclearRT Banner](assets/banner.png)

<p align="center">
    <a href="https://discord.gg/dgEjYe7UQa">
        <img src="https://img.shields.io/badge/Discord-Join%20Server-5865F2?logo=discord&logoColor=white&style=for-the-badge" alt="Discord Server">
    </a>
</p>

> [!CAUTION]
> This project is still in development and is not ready for general use.

## Features
- Faster than the default runtime.
- Provides 64-bit support
- Native Linux & macOS support

## Limitations
- Any missing extension will need to be rewritten.
- Sub-Applications are not supported.

## Supported Platforms
- Windows
- Linux
- macOS (Intel & Apple Silicon)
- Web
- Switch (Homebrew)

## Supported Backends
- SDL3
- SDL2

## Requirements
- Clickteam Fusion 2.5 R295.10 or higher
- CMake 3.14 or higher
- A C++ compiler, such as MSVC or GCC
- Web:
  - Emscripten
- Switch:
  - DevkitPro, SDL2 Portlibs

## Usage

Release builds are still not ready. You can build the application manually by following the instructions in the [Development](#development) section.

## Development

Working on NuclearRT has a bit of a strange workflow. The easiest way to do it is:
1. Clone the repository.
2. Create a Symbolic Link for the [exporter](/exporter) from `nuclearrt\exporter\bin\Debug\net9.0-windows` to `Clickteam Fusion 2.5\Data\Runtime\nuclearrt\exporter`
3. Create a Symbolic Link for the base [runtime](/runtime) from `nuclearrt\runtime` to `Clickteam Fusion 2.5\Data\Runtime\nuclearrt\runtime`
4. Build the exporter [plugin](/plugin) and copy the `nuclearrt.bld` file to `Clickteam Fusion 2.5\Data\Runtime\`

If you make any changes to the exporter:
1. `dotnet build` in the `exporter` directory.
2. Build the application in Fusion.

If you make any changes to the base runtime:
1. Build the application in Fusion.

## Contributing

Contributions are welcome! Please open an issue or pull request to contribute.

## License

This project is licensed under the AGPL-3.0 license. See the [LICENSE](LICENSE) file for details.

## Credits

- [Clickteam](https://www.clickteam.com/) for making Fusion.
- [MP2](https://www.mp2.dk/) for making Chowdren and inspiring me to make this runtime and making the Fusion plugin.
- [CTFAK](https://github.com/CTFAK) for making the decompiler used in this project.
