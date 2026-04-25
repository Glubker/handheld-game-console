# Handheld Game Console

A personal project documenting the design and build of a fully custom 
handheld game console from scratch. This repository contains all source 
code, PCB design files, and supporting materials for the build. It is 
not a polished open source project, but a record of something I built 
for myself that others are welcome to learn from and adapt.

The current build runs on a Radxa Rock 5C with an 8" 1280x800 display, 
housed in a custom 3D printed casing with a custom PCB handling all 
physical input. The graphical interface and game launcher were written 
entirely from scratch in Flutter. Physical buttons are mapped to standard 
keypresses by a dedicated background program, meaning any software that 
accepts keyboard input runs on the device without modification.

Full documentation covering the hardware, PCB, software architecture, 
OS configuration, and complete build process is available at  
**[lubker.dk/gameconsole](https://lubker.dk/gameconsole)**

---

## Repository Structure

    handheld-game-console/
    ├── pcb/                          # KiCad PCB design files and Gerbers
    ├── raspberry-pi/                 # Original build: Raspberry Pi 4 + C++/Raylib
    │   ├── game_console/             # Main interface and built-in games
    │   │   ├── main.cpp
    │   │   ├── src/                  # UI components and utilities
    │   │   ├── resources/            # Fonts and icons
    │   │   └── games/game_src/       # Source for built-in games
    │   │       ├── pong/
    │   │       ├── dodgers/
    │   │       ├── flappy_bird/
    │   │       ├── pacman/
    │   │       └── platformer/
    │   ├── gpio_testing.py           # GPIO input testing script (gpiozero)
    │   └── gpio_to_keypress.c        # Input handler (WiringPi)
    └── radxa-rock-5c/                # Current build: Radxa Rock 5C + Flutter
        ├── flutter_src/              # Flutter interface source
        │   └── lib/
        │       ├── main.dart
        │       ├── models/           # Game model, keyboard manager, settings
        │       ├── screens/          # Home screen
        │       ├── widgets/          # UI components
        │       └── utils/
        ├── gpio_testing.py           # GPIO input testing script (gpiod)
        ├── gpio_to_keypress.c        # Input handler (libgpiod)
        └── launcher.sh               # Launch script

---

## License

MIT. See [LICENSE](LICENSE) for details.

If you use this project as a reference or build upon it, a credit or link 
back to [lubker.dk/gameconsole](https://lubker.dk/gameconsole) is appreciated.
