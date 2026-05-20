# Shear

Shear is a Windows x64 VST3 distortion plugin built with JUCE. It is designed around a sharp, split-waveform visual identity and a simple control surface for cutting, folding, clipping, and crushing incoming audio.

## Features

- Four distortion modes: Warm, Hard, Fold, and Crush.
- Input, Drive, Tone, Bias, Mix, and Output controls.
- HQ mode with an interpolated shaping path for smoother high-drive edges.
- Input and output RMS meters.
- Animated transfer-curve display.
- Host state save/restore through JUCE's `AudioProcessorValueTreeState`.
- Modern dark UI with a Shear wordmark and diagonal cut logo.

## Download

Download the latest release asset named like:

```text
Shear-v1.0.0-windows-vst3.zip
```

Extract it and copy `Shear.vst3` to:

```text
C:\Program Files\Common Files\VST3
```

Then open FL Studio and rescan plugins.

If you previously installed an older build under `SkeuoDrive.vst3` or `Emberfold.vst3`, remove those old bundles from the VST3 folder before rescanning so the host does not cache duplicate plugin entries.

## Build From Source

Requirements:

- Windows x64.
- Visual Studio 2022 with the Desktop development with C++ workload.
- JUCE 8.x modules available at the path configured in `NewProject.jucer` and the generated Visual Studio project files.

The current generated project expects JUCE modules at:

```text
D:\Programs\JUCE\modules
```

To build the VST3 from PowerShell:

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\amd64\MSBuild.exe' 'Builds\VisualStudio2026\NewProject.sln' /t:"NewProject - VST3" /p:Configuration=Release /p:Platform=x64 /m
```

The built VST3 appears at:

```text
Builds\VisualStudio2026\x64\Release\VST3\Shear.vst3
```

## Source Layout

```text
Source/
  PluginProcessor.*   DSP, parameters, state, meters
  PluginEditor.*      Custom JUCE UI and controls
JuceLibraryCode/      Projucer-generated JUCE include wrappers
Builds/               Generated Visual Studio project files
NewProject.jucer      JUCE project definition
```

## Release Notes

### v1.0.0

- Initial Windows x64 VST3 release.
- Added four distortion modes, HQ mode, tone shaping, mix/output gain, meters, and custom Shear UI.

## License

Shear is released under the GNU Affero General Public License v3.0. See [LICENSE](LICENSE).

JUCE is dual-licensed under AGPLv3 or a commercial JUCE license. This project follows JUCE's open-source licensing route and does not include the JUCE framework source itself.
