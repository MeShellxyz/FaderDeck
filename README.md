# FaderDeck

<div align="center">
  <img src="pc-app/src/WinFun/icons/icon.svg" alt="FaderDeck Logo" width="120" height="120">
  <br><br>
  
  [![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
  [![Platform: Windows (Cross-platform planned)](https://img.shields.io/badge/Platform-Windows-0078D6.svg)]()
  [![Language: C++20](https://img.shields.io/badge/Language-C%2B%2B20-00599C.svg)]()
  [![Build: CMake](https://img.shields.io/badge/Build-CMake-064F8C.svg)]()
</div>

**FaderDeck** is a native, highly optimized hardware audio mixer. It bridges an Arduino-based physical slider deck directly with your operating system's audio API, allowing you to control system master volume and per-application audio levels using physical potentiometers.

Designed with strict performance constraints, FaderDeck runs silently in the system tray, consuming **< 3MB of RAM** and practically **0% CPU** while idle.

## 🧰 Tech Stack
* **Language:** C++20
* **System APIs:** Windows Core Audio API (WASAPI), Win32 API, Windows Registry
* **Libraries:** Boost.Asio (Serial/Async I/O), toml++ (Configuration)
* **Hardware:** Arduino (C++) / ADC Serial Communication
* **Build System:** CMake

## ✨ Key Features
- **Tactile Audio Control:** Adjust master volume or specific apps (Spotify, Chrome, Games) natively without alt-tabbing.
- **Zero-Cost Idle:** Event-driven architecture ensures the app only consumes CPU cycles when a physical slider is moved or a new audio session is launched.
- **Hardware Noise Filtering:** Built-in ADC clamping and signal debouncing for stable, jitter-free volume control.
- **Portable & Standalone:** Can be statically linked into a single lightweight `.exe`.

---

## 🚀 Quick Start (User Guide)

### 1. Hardware Setup
To build the physical deck, you will need an Arduino (Uno, Nano, or similar) and standard linear potentiometers.



**Wiring Guide:**
- **Outer Pin 1:** Connect to Arduino `5V`
- **Outer Pin 2:** Connect to Arduino `GND`
- **Middle Pin (Wiper):** Connect to Analog Pins (`A0`, `A1`, `A2`, etc.)

*Flash the provided `mcu/faderdeck/faderdeck.ino` sketch to your Arduino using the Arduino IDE.*

### 2. Software Configuration
FaderDeck uses a simple `config.toml` file to map your physical hardware channels to specific Windows executables. Place this file in the exact same directory as the executable.

```toml
[system]
auto_start = true

[serial]
com_port = "COM3"
baud_rate = 115200
invert_sliders = false

[audio]
mute_buttons = false
num_channels = 2

# Channel 0 controls System Master Volume
[[audio.channels]]
channel = 0
processes = ["master"]

# Channel 1 dynamically links to specific apps
[[audio.channels]]
channel = 1
processes = ["spotify.exe", "chrome.exe"]
```

## 📋 Prerequisites & Requirements

Since FaderDeck deeply integrates with the Windows Native APIs, it is currently tested and supported only on Windows using the MSVC compiler. Ensure your development environment meets the following requirements:

* **Compiler:** MSVC v143+ (Visual Studio 2022) with C++20 support.
* **System SDK:** Windows 10/11 SDK (required for WASAPI and Win32 headers).
* **Build System:** [CMake](https://cmake.org/download/) (v3.23 or higher is **required** for CMake Presets support).
* **Package Manager:** [Conan](https://conan.io/downloads) (v2.x) to automatically handle dependencies like `Boost.Asio` and `toml++`.
* **Hardware Setup:** [Arduino IDE](https://www.arduino.cc/en/software) for flashing the microcontroller firmware.

## 🛠️ Building from Source (CMake + Conan Presets)

This project uses **Conan 2** to seamlessly manage C++ dependencies and generates **CMake Presets** for a frictionless build process.

```bash
# 1. Clone the repository
git clone [https://github.com/YourUsername/FaderDeck.git](https://github.com/YourUsername/FaderDeck.git)
cd FaderDeck/pc-app

# 2. Install dependencies and generate CMake presets using Conan
# (Assuming you have a default MSVC conan profile set up)
conan install . --output-folder=build --build=missing

# 3. Configure the project using the Conan-generated preset
cmake --preset conan-default

# 4. Build the executable in Release mode
cmake --build --preset conan-release
```

## 🙏 Acknowledgments
Inspired by the excellent [deej](https://github.com/omriharel/deej) project. FaderDeck was written to provide a strictly native C++ alternative focusing on zero-cost abstractions, memory efficiency, modern C++ package management, and robust Win32 API integration.

## 📜 License
Released under the MIT License. See [LICENSE](LICENSE) for details.

---
<div align="center">

  <sub>Built by <a href="https://github.com/MeShellxyz">>MeShell</a></sub>

</div>