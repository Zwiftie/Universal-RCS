# Universal RCS – Recoil Control System

> **Advanced, low‑latency recoil compensation for PC shooters**  
> *Fully animated ImGui interface · Global input hooks · Game‑specific profiles*

![Status](https://img.shields.io/badge/version-1.0-blue) ![License](https://img.shields.io/badge/license-MIT-green) ![Windows](https://img.shields.io/badge/platform-Windows-0078d7)

**Universal RCS** is a standalone Windows application that automatically counteracts weapon recoil in first‑person shooter games. It works by injecting synthetic mouse movements based on user‑defined or preset recoil patterns – without reading or writing game memory. The result is a universal, non‑invasive tool that can be used with any FPS title.

---

## ✨ Features

| Category | Details |
|----------|---------|
| **Recoil control** | Independent sliders for **Left / Right / Up / Down** compensation (0–100). Sensitivity, smoothing, and start‑up delay. |
| **Activation modes** | Toggle key (customisable), hold M1, or hold M1+M2. |
| **Burst fire** | 3‑round or 5‑round burst – automatically disables after the set number of shots. |
| **Advanced options** | Randomisation (±10%) to avoid pattern detection. Adaptive time‑based scaling (gradually increases over 2 seconds). |
| **Game profiles** | One‑click presets for **Rainbow Six**, **CS2**, **Valorant**, **Apex Legends**. Fully customisable for any other game. |
| **Modern GUI** | Smooth animations, glowing accents, dark glass theme. Built with **Dear ImGui** and rendered via **DirectX 11**. |
| **Configuration** | Save/load settings to `recoil.cfg`. Persistent across sessions. |
| **Global operation** | Works in any foreground window – no game injection or memory patching. |

---

## 🖼️ UI Preview

```
┌─────────────────────────────────────────────────┐
│                  RECOIL MASTER                  │
│             Advanced Recoil Control             │
├─────────────────────────────────────────────────┤
│  ● ACTIVE                             TOGGLE    │
├─────────────────────────────────────────────────┤
│  Game Profile:  [Custom ▼]                      │
│  ─────────────────────────────────────────────  │
│  Recoil Compensation                            │
│  Left      [━━━━━━━━━━━━━━━━━━━━] 0             │
│  Right     [━━━━━━━━━━━━━━━━━━━━] 0             │
│  Up        [━━━━━━━━━━━━━━━━━━━━] 0             │
│  Down      [━━━━━━━━━━━━━━━━━━━━] 0             │
│  ─────────────────────────────────────────────  │
│  Sensitivity  [━━━━━━━━━━━━━━━━━━━━] 10.0       │
│  Smoothing    [━━━━━━━━━━━━━━━━━━━━] 50%        │
│  Delay        [━━━━━━━━━━━━━━━━━━━━] 0ms        │
│  ─────────────────────────────────────────────  │
│  Randomize (±10%)      [ OFF  ]                 │
│  Adaptive Time‑Based    [ OFF  ]                 │
│  Burst Fire            [Disabled ▼]             │
│  Activation            [Toggle Key ▼]           │
├─────────────────────────────────────────────────┤
│  [ Save Config ]  [ Load Config ]  [ Set Key ]  │
└─────────────────────────────────────────────────┘
```

> *Actual UI includes smooth slider handles, pulsing status light, and real‑time burst counter.*

---

## 🔧 How It Works

The application installs low‑level Windows hooks (`WH_KEYBOARD_LL` and `WH_MOUSE_LL`) to monitor global input. When the activation condition is met (e.g., toggle key pressed or mouse button held), it calculates the required mouse displacement based on the configured recoil pattern, applies smoothing and randomisation, and sends relative mouse movements via `SendInput`.

All calculations are performed in real time, using fractional accumulation to preserve sub‑pixel precision. No game memory is read or modified – the tool only simulates mouse input.

---

## 🚀 Getting Started

### Prerequisites

- **Windows 10 / 11** (64‑bit recommended)
- Visual Studio 2022 (or any C++ compiler with Windows SDK)
- [Dear ImGui](https://github.com/ocornut/imgui) (included as submodule or manually)
- DirectX 11 runtime (included in Windows)

### Building

1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/Universal-RCS.git
   cd Universal-RCS
   ```
2. **Set up ImGui** – Copy the following ImGui files into the project directory:
   - `imgui.h`, `imgui.cpp`
   - `imgui_draw.cpp`, `imgui_widgets.cpp`, `imgui_tables.cpp`
   - `backends/imgui_impl_win32.h`, `backends/imgui_impl_win32.cpp`
   - `backends/imgui_impl_dx11.h`, `backends/imgui_impl_dx11.cpp`
3. Open the solution in Visual Studio.
4. Build as **x64** (the hooks require 64‑bit).
5. Run the executable **as administrator** (required for low‑level hooks).

### Configuration

- **Key binding**: Click `Set Key` and press any key – that becomes the new toggle key.
- **Profiles**: Select a game from the dropdown to load recommended values.
- **Saving**: Use the `Save Config` button – settings are stored in `recoil.cfg` in the same folder.
- **Manual tuning**: Adjust the directional sliders while testing in‑game to match the exact recoil pattern.

> ⚠️ **Note**: The low‑level hooks require administrative privileges. Right‑click the `.exe` → *Run as administrator*.

---

## ⚖️ Disclaimer

**This software is provided for educational purposes only.**  
Using input‑simulation tools in online multiplayer games may violate the game’s Terms of Service and can result in permanent account bans. The author assumes no liability for any consequences arising from the use of this program. **Do not use it in competitive or anti‑cheat protected environments unless explicitly allowed.**

---

## 🛠️ Technical Details

- **Language**: C++20
- **Graphics API**: DirectX 11
- **UI Library**: Dear ImGui (custom styling, no additional dependencies)
- **Hooks**: `WH_KEYBOARD_LL`, `WH_MOUSE_LL`
- **Fonts** (optional): `BAUHS93.TTF`, `impact.ttf`, `segoeui.ttf`, `calibri.ttf`, `consola.ttf` – falls back to default if missing.
- **Persistence**: Plain text INI‑style config file.

### Algorithm Pseudocode

```cpp
if (active && !burstLimitReached && delayElapsed) {
    rawDX = (right - left) / sensitivity;
    rawDY = (down - up) / sensitivity;
    
    if (randomize) applyRandomOffset();
    if (adaptive) scaleOverTime();
    
    target = lerp(target, raw, 1 - smoothing);
    accum += target;
    
    SendInput(floor(accum.x), floor(accum.y));
    accum -= floor(accum);
}
```

---

## 📝 License

Distributed under the **MIT License**. See `LICENSE` for more information.

---

## 🙏 Acknowledgements

- [Dear ImGui](https://github.com/ocornut/imgui) – immediate mode GUI
- Microsoft – DirectX 11 and Windows Hook APIs
- The FPS gaming community for recoil pattern insights

---

## 📫 Contact & Contributions

Pull requests and feature suggestions are welcome.  
Please open an issue first to discuss major changes.
Discord : snofla.cpp

---

*Made with ❤️ for the love of precision aiming.*
