![Ship of Harkinian](docs/shiptitle.darkmode.png#gh-dark-mode-only)
![Ship of Harkinian](docs/shiptitle.lightmode.png#gh-light-mode-only)

## Website

Official Website: https://www.shipofharkinian.com/

## Discord

Official Discord: https://discord.com/invite/shipofharkinian

If you're having any trouble after reading through this `README`, feel free to ask for help in the Support text channels. Please keep in mind that we do not condone piracy.

# Quick Start

The Ship does not include any copyrighted assets.  You are required to provide a supported copy of the game.

### 1. Verify your ROM dump
You can verify you have dumped a supported copy of the game by using the compatibility checker at https://ship.equipment/. If you'd prefer to manually validate your ROM dump, you can cross-reference its `sha1` hash with the hashes [here](docs/supportedHashes.json).

### 2. Download The Ship of Harkinian from [Releases](https://github.com/HarbourMasters/Shipwright/releases)

### 3. Launch the Game!
#### Windows
* Extract the zip
* Launch `soh.exe`

#### Linux
* Place your supported copy of the game in the same folder as the appimage.
* Execute `soh.appimage`.  You may have to `chmod +x` the appimage via terminal.

#### macOS
* Run `soh.app`. When prompted, select your supported copy of the game.
* You should see a notification saying `Processing OTR`, then, once the process is complete, you should get a notification saying `OTR Successfully Generated`, then the game should start.

#### Nintendo Switch
* Run one of the PC releases to generate an `oot.o2r` and/or `oot-mq.o2r` file. After launching the game on PC, you will be able to find these files in the same directory as `soh.exe` or `soh.appimage`. On macOS, these files can be found in `/Users/<username>/Library/Application Support/com.shipofharkinian.soh/`
* Copy the files to your sd card
```
sdcard
└── switch
    └── soh
        ├── oot-mq.o2r
        ├── oot.o2r
        ├── soh.nro
        └── soh.o2r
```
* Launch via Atmosphere's `Game+R` launcher method.

### 4. Play!

Congratulations, you are now sailing with the Ship of Harkinian! Have fun!

# Configuration

This accessibility build uses the game's Options screen for all port settings. Open it from the title screen,
file selection, the Options pause tab, or Escape. The pause tab sits between Equipment and Items.
Use your mapped stick or D-pad to navigate, A to select, B to return, and left/right to adjust settings.
R reads the complete description. Keyboard controls are arrow keys, Enter, Backspace and F1 for details.

Prism speaks the focused setting, its value, description and position. Settings are grouped under Accessibility,
Audio, Display, Controls, Gameplay, Cosmetics, Randomizer, Trackers, Network, System and Advanced. Search is the
first entry. Developer tools are under Advanced; tracker overlays keep their existing layouts.

Controller navigation follows the game bindings. To remap, select a binding, release all inputs, then press the new
input. Escape cancels capture, and an idle capture cancels after ten seconds. If you clear your controller bindings,
use the keyboard to restore them under Controls, Configure Controller, Link (P1), Set Defaults.

### Default keyboard configuration
| N64 | A | B | Z | Start | Analog stick | C buttons | D-Pad |
| - | - | - | - | - | - | - | - |
| Keyboard | X | C | Z | Space | WASD | Arrow keys | TFGH |

### Other shortcuts
| Keys | Action |
| - | - |
| ESC | Toggle menu |
| F2 | Toggle capture mouse input |
| F5 | Save state |
| F6 | Change state |
| F7 | Load state |
| F11 | Fullscreen |
| Tab | Toggle Alternate assets |
| Ctrl+R | Reset |

# Project Overview
Ship of Harkinian (SOH) is built atop a custom library dubbed libultraship (LUS). Back in the N64 days, there was an SDK distributed to developers named libultra; LUS is designed to mimic the functionality of libultra on modern hardware. In addition, we are dependant on the source code provided by the OOT decompilation project.

In order for the game to function, you will require a **legally acquired** ROM for Ocarina of Time. Click [here](https://ship.equipment/) to check the compatibility of your specific rom. Any copyrighted assets are extracted from the ROM and reformatted as a .o2r archive file which the code uses.

### Graphics Backends
Rendering APIs include DirectX11, OpenGL and Metal, depending on the platform. Select the renderer in Options,
Display and restart the game to apply it. DirectX 11 is the default on Windows.

# Custom Assets

Custom assets are packed in `.otr` archive files. To use custom assets, place them in the `mods` folder.

If you're interested in creating and/or packing your own custom asset `.otr` files, check out the following tools:
* [**retro - OTR generator**](https://github.com/HarbourMasters64/retro)
* [**fast64 - Blender plugin**](https://github.com/HarbourMasters/fast64)

# Development
### Building

If you want to manually compile SoH, please consult the [building instructions](docs/BUILDING.md).

### Playtesting
If you want to playtest a continuous integration build, you can find them at the links below. Keep in mind that these are for playtesting only, and you will likely encounter bugs and possibly crashes. 

* [Windows](https://nightly.link/HarbourMasters/Shipwright/workflows/generate-builds/develop/soh-windows.zip)
* [macOS](https://nightly.link/HarbourMasters/Shipwright/workflows/generate-builds/develop/soh-mac.zip)
* [Linux](https://nightly.link/HarbourMasters/Shipwright/workflows/generate-builds/develop/soh-linux.zip)

### Further Reading
More detailed documentation can be found in the 'docs' directory, including the aforementioned [building instructions](docs/BUILDING.md).

* [Credits](docs/CREDITS.md)
* [Custom Music](docs/CUSTOM_MUSIC.md)
* [Controller Mapping](docs/GAME_CONTROLLER_DB.md)
* [Modding](docs/MODDING.md)
* [Versioning](docs/VERSIONING.md)

<a href="https://github.com/Kenix3/libultraship/">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="./docs/poweredbylus.darkmode.png">
    <img alt="Powered by libultraship" src="./docs/poweredbylus.lightmode.png">
  </picture>
</a>
