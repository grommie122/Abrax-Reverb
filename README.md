# Abrax Reverb

[Download Windows x64 VST3 ZIP](https://github.com/grommie122/Abrax-Reverb/releases/download/rolling-windows/Abrax-Reverb-Windows-x64.zip)

GitHub Actions refreshes this ZIP after successful builds from `main`.


Abrax Reverb is a Windows x64 hall reverb with Size, Damping, Width, Mix, and Freeze controls.

## Build

Install CMake and Visual Studio with Desktop development with C++. From
PowerShell in this folder, run:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

CMake fetches JUCE. To build with an existing JUCE checkout, add
`-DJUCE_DIR=C:/path/to/JUCE` to the first command.

## Install in REAPER

Copy the entire `Abrax Reverb.vst3` folder from
`build/AbraxReverb_artefacts/Release/VST3` into a directory in REAPER's VST
search path. On this machine the per-user path is
`%LOCALAPPDATA%\Programs\Common\VST3`.
In REAPER, open Preferences > Plug-ins > VST and click Re-scan. On a track,
open FX and search for Abrax Reverb.

