# Elaw Audio Suite

Elaw Audio Suite is a public Windows x64 VST3 plugin collection built for fun, learning, and portfolio work. The suite uses JUCE 8, React/WebView editors, and a shared dark geometric visual system.

## Plugins

| Plugin | Type | Release asset |
| --- | --- | --- |
| Shear | Waveform distortion shaper | `Shear-v1.2.0-windows-vst3.zip` |
| Tilt | Tone tilt and character EQ | `Tilt-v1.0.0-windows-vst3.zip` |
| Clamp | Compressor and limiter with character | `Clamp-v1.0.0-windows-vst3.zip` |
| Drift | Modulated delay and widening effect | `Drift-v1.0.0-windows-vst3.zip` |
| Rift | Experimental grain and freeze effect | `Rift-v1.0.0-windows-vst3.zip` |

## Install

Download a plugin release zip, extract it, and copy the `.vst3` bundle to:

```text
C:\Program Files\Common Files\VST3
```

Then open FL Studio and rescan plugins.

## Build From Source

Requirements:

- Windows x64.
- Visual Studio 2022 with Desktop development with C++.
- Node.js 22 or newer.
- JUCE 8.0.13 modules at `D:\Programs\JUCE\modules`.

Build every WebView UI and VST3:

```powershell
npm install
powershell -ExecutionPolicy Bypass -File scripts/build-all.ps1
```

Build one plugin:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build-plugin.ps1 -Plugin Shear
```

Package one plugin:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/package-plugin.ps1 -Plugin Shear -Version 1.2.0
```

## Source Layout

```text
plugins/<Name>/Source/        JUCE processor/editor for each plugin
plugins/<Name>/web-ui/        Product-specific React entry/config
shared/cpp/ElawAudio/         Shared JUCE WebView/editor helpers
shared/web/                   Shared React UI, styles, and JUCE bridge
scripts/                      Build and packaging scripts
```

## License

Elaw Audio Suite is released under the GNU Affero General Public License v3.0. See [LICENSE](LICENSE).

JUCE is dual-licensed under AGPLv3 or a commercial JUCE license. This project follows JUCE's open-source licensing route and does not include the JUCE framework source itself.
