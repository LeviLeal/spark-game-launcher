<p align="center">
  <img src="logo.png" alt="Spark Logo" width="128" height="128" />
</p>

<h1 align="center">Spark Game Launcher</h1>

<p align="center">
  A fast, lightweight, and modern Linux game launcher with native Proton & Wine runner support.
</p>

<p align="center">
  <a href="#features">Features</a> •
  <a href="#prerequisites">Prerequisites</a> •
  <a href="#building-from-source">Building</a> •
  <a href="#packaging">Packaging</a> •
  <a href="#desktop-integration">Desktop Integration</a> •
  <a href="#license">License</a>
</p>

---

## Features

- 🚀 **Fast & Lightweight**: Built with C++17 and modern Qt 6 for instant startup and minimal memory usage.
- 🍷 **Wine & Proton Integration**: Automatic runner detection across system paths, Steam, and custom prefixes.
- 📥 **Integrated Proton Downloader**: Fetch and manage Proton releases directly from the launcher.
- 🖥️ **Smart Desktop Integration**: Installs a `.desktop` launcher in `~/.local/share/applications` that intelligently tracks the executable's current location even if moved.
- 🔒 **Single-Instance Enforcement**: Prevents multiple background processes from clashing; clicking Spark when already open brings the active window to focus.
- 📦 **Zero-Bloat Portable Distribution**: Easily creates standalone `.tar.xz` archives with no external runtime dependencies beyond system Qt 6.

---

## Prerequisites

To build Spark, you need a C++17 compiler, CMake, and Qt 6 (`Widgets` and `Network` components).

### Arch Linux / Manjaro
```bash
sudo pacman -S base-devel cmake qt6-base
```

### Ubuntu / Debian (22.04+)
```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev libqt6widgets6 libqt6network6
```

### Fedora
```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel
```

---

## Building from Source

### Quick Build (Recommended)
Use the included helper script:

```bash
# Build in Release mode (default)
./scripts/build.sh

# Build in Debug mode
./scripts/build.sh --debug

# Clean build directory before compiling
./scripts/build.sh --clean
```

### Manual CMake Build
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

The compiled binary will be placed at `build/spark`.

### Running
```bash
./build/spark
```

---

## Packaging

Spark includes a packaging script that produces a clean, self-contained `.tar.xz` archive containing the binary, a launcher script, icons, and desktop entries:

```bash
# Generate the portable tar.xz package:
./scripts/build-tarball.sh

# Or via CMake:
cmake --build build --target tarball
```

The output archive will be saved in `build/spark-<version>-linux-<arch>.tar.xz`.

---

## Desktop Integration

If you extract the portable archive anywhere on your system (e.g. `~/Games/Spark` or `~/.local/bin`):

1. Launch Spark (`./spark` or double-click `spark.sh`).
2. Click the **Settings** icon on the top toolbar.
3. In the **Desktop Integration** tab, click **"Add to System Menu (.desktop)"**.

Spark will automatically:
- Extract its high-resolution icon to `~/.local/share/icons/hicolor/512x512/apps/spark.png`.
- Write `spark.desktop` to `~/.local/share/applications/spark.desktop` pointing directly to the current executable path.
- Refresh the system desktop database (`update-desktop-database`) so Spark appears immediately in your application launcher (GNOME, KDE Plasma, XFCE, etc.).

---

## Project Structure

```text
spark-game-launcher/
├── CMakeLists.txt              # CMake build configuration
├── logo.png                    # Application logo (512x512)
├── resources.qrc               # Qt resource bundle
├── scripts/
│   ├── build.sh                # Build automation script
│   └── build-tarball.sh        # Portable tar.xz packaging script
└── src/
    ├── main.cpp                # Entry point & single-instance handler
    ├── mainwindow.cpp / .h     # Main launcher interface
    ├── dialogs/
    │   ├── gameconfigdialog    # Add/Edit game configuration
    │   ├── generalsettings     # Desktop integration settings pane
    │   ├── logviewerdialog     # Runner log viewer
    │   ├── protondownload      # Proton version downloader
    │   └── settingsdialog      # Stacked settings dialog
    ├── models/                 # Data structures
    └── utils/                  # Library, process, and runner managers
```

---

## License

This project is open-source. See the repository for license details.
