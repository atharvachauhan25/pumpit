# PumpIt

PumpIt is a free, lightweight sidechain ducking VST plugin inspired by Nicky Romero's Kickstart and Flux Mini 2. 
It offers perfectly tempo-synced volume modulation to create that classic pumping effect in your tracks without complex routing.

## Features

- **15 Pre-designed Curves**: Get started instantly with a massive library of volume shapes (Standard, Tight, Gate, Sine, Staircase, Double Pump, and more).
- **Interactive Custom Curve Editor**: Draw your own precise volume envelopes! Double-click to add or remove points, and drag the visible tension handles to create perfect exponential or logarithmic bends.
- **Anti-Click DSP**: Built-in 1-pole envelope filtering and mathematical fade-outs completely prevent audio popping and clicking at loop boundaries.
- **Live Playback Mode**: When your DAW transport is stopped, the plugin seamlessly detaches from the grid and uses a free-running phase, allowing you to hear the sidechain effect while playing your MIDI instruments live.
- **Tempo Synced**: Always stays perfectly locked to your DAW's host BPM and PPQ when playing.
- **Beat Divisions**: Select from standard rhythmic subdivisions (1/1, 1/2, 1/4, 1/8, 1/16, 1/32).
- **Mix Control**: Dial in the exact amount of ducking using the Dry/Wet mix knob.
- **Lightweight**: Extremely low CPU footprint, perfect for dropping onto dozens of tracks in large mixing sessions.

## Formats Supported
- VST3 (Windows/macOS)
- AU (macOS)
*Note: Currently built and tested on VST3 for Windows, with more builds coming later.*

## Installation (For Users)

1. Download the latest `PumpIt.vst3` file from the [Releases](https://github.com/atharvachauhan25/pumpit/releases) page.
2. **Windows**: Copy `PumpIt.vst3` to `C:\Program Files\Common Files\VST3`
3. **macOS**: Copy `PumpIt.vst3` to `/Library/Audio/Plug-Ins/VST3` (and/or the `Components` folder for AU).
4. Rescan your plugins in your DAW. That's it!

## Build Instructions (For Developers)

This project uses modern CMake and the [JUCE](https://juce.com/) framework. JUCE is fetched automatically via CMake, so there is no need to manually install it.

### Prerequisites
- CMake (3.15 or higher)
- A modern C++ compiler (Visual Studio Build Tools for Windows, Xcode for macOS)

### Building
1. Clone the repository:
   ```bash
   git clone https://github.com/atharvachauhan25/pumpit.git
   cd pumpit
   ```
2. Generate the build files:
   ```bash
   cmake -B build
   ```
3. Build the plugin:
   ```bash
   cmake --build build --config Release
   ```
4. Find the built plugin in the `build` directory and copy it to your DAW's VST3 folder (e.g., `C:\Program Files\Common Files\VST3` on Windows).

## License
Open Source - Free to use.
