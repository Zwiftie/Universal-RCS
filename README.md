# Universal RCS – Classic Win32 Edition

> **Lightweight, low‑level recoil control with native Windows UI**  
> *Trackbar‑based compensation · Global hotkeys · Hold‑to‑activate modes*

![Version](https://img.shields.io/badge/version-1.0-blue) ![License](https://img.shields.io/badge/license-MIT-green) ![Platform](https://img.shields.io/badge/platform-Windows-0078d7) ![UI](https://img.shields.io/badge/UI-Win32%20Native-008080)

**Universal RCS (Classic)** is a minimal, dependency‑free recoil compensation tool for PC shooters. It uses the native Windows API (no external libraries) and low‑level keyboard/mouse hooks to simulate counter‑recoil movements. The classic trackbar interface provides precise, real‑time control with zero overhead.

---

## ✨ Features

| Category | Details |
|----------|---------|
| **Recoil compensation** | Four independent sliders for **Left**, **Right**, **Up**, **Down** (0–100). Sensitivity multiplier (1.0–10.0). |
| **Activation modes** | **Toggle key** (customizable), **Hold Mouse 1**, or **Hold Mouse 1+2**. |
| **Global hotkey** | Set any key as the toggle – works while the app is in the background. |
| **Real‑time adjustment** | Move sliders while the game is running – changes apply immediately. |
| **Configuration** | Save/load profiles via standard file dialogs (`.ini` format). “New Config” resets to defaults. |
| **Sub‑pixel precision** | Accumulates fractional movements to maintain accuracy over time. |
| **Native Win32 UI** | No external runtimes – just a small, fast executable. |

---

## 🖼️ UI Preview

```
┌─────────────────────────────────────────────────┐
│ Recoil Control                            [−][□][×]│
├─────────────────────────────────────────────────┤
│ Left:      [━━━━━━━━━━━━━━━━━━━━] 0              │
│ Right:     [━━━━━━━━━━━━━━━━━━━━] 0              │
│ Up:        [━━━━━━━━━━━━━━━━━━━━] 0              │
│ Down:      [━━━━━━━━━━━━━━━━━━━━] 0              │
│ Sensitivity:[━━━━━━━━━━━━━━━━━━━━] 1.0           │
│                                                  │
│ [ Set Toggle Key ]   Key: F2                    │
│                                                  │
│ Activation Mode:     [Toggle Key ▼]             │
│                                                  │
│ Status: Inactive                                │
│                                                  │
│ [ Save Config ]  [ Load Config ]  [ New Config ]│
└─────────────────────────────────────────────────┘
```

> *Actual window size: 430×440 pixels. All controls update live.*

---

## 🔧 How It Works

The application installs two low‑level Windows hooks:
- **`WH_KEYBOARD_LL`** – detects the toggle key (in “Toggle Key” mode) or captures a new key when the user clicks “Set Toggle Key”.
- **`WH_MOUSE_LL`** – monitors left and right button states for the hold modes.

When the activation condition is met:
- A 10ms timer (`IDT_MOUSE_MOVE`) calculates the required mouse movement:
  ```
  deltaX = (right - left) / (sensitivity / 10)
  deltaY = (down - up)   / (sensitivity / 10)
  ```
- Fractional parts are accumulated – each full integer pixel is sent via `SendInput` as a relative mouse move.
- The process repeats until the activation condition ends.

No game memory is read or modified – only synthetic mouse input is generated.

---

## 🚀 Getting Started

### Prerequisites

- **Windows 7 / 8 / 10 / 11** (32‑bit or 64‑bit)
- **Visual Studio** (any version with C++ and Windows SDK) or MinGW
- No special libraries – uses only `windows.h`, `commctrl.h`, `commdlg.h`.

### Building

1. Clone or download the source.
2. Open the project in Visual Studio.
3. Create a new **Windows Desktop Application** project (or a plain Win32 project).
4. Add the provided `.cpp` file to the project.
5. Set the subsystem to **Windows** (`/SUBSYSTEM:WINDOWS`).
6. Build as **Release** for best performance.
7. Run the executable **as administrator** (required for low‑level hooks).

### First Run

- The default toggle key is **F2**.  
- To change it, click `Set Toggle Key` and press any key.  
- Select the activation mode from the dropdown:  
  *Toggle Key* – press your key to enable/disable.  
  *Mouse 1* – hold left mouse button.  
  *Mouse 1+2* – hold both left and right buttons.  
- Adjust sliders while testing in‑game – the compensation is applied every 10ms.

### Saving & Loading

- Use `Save Config` – choose a location and filename (e.g., `rainbow.ini`).  
- `Load Config` – load a previously saved `.ini` file.  
- `New Config` – resets all values to zero and sensitivity to 1.0.

> ⚠️ **Note**: Low‑level hooks require **administrator privileges**. Right‑click the `.exe` → *Run as administrator*.

---

## ⚖️ Disclaimer

**This software is provided for educational purposes only.**  
Using input‑simulation tools in online multiplayer games may violate the game’s Terms of Service and can result in permanent account bans. The author assumes no liability for any consequences arising from the use of this program. **Do not use it in competitive or anti‑cheat protected environments unless explicitly allowed.**

---

## 🛠️ Technical Details

- **Language**: C++17 (Win32 API)
- **UI**: Native Windows controls (trackbars, buttons, static text, combobox)
- **Hooks**: `WH_KEYBOARD_LL`, `WH_MOUSE_LL`
- **Configuration**: Windows INI file API (`WritePrivateProfileStringW`, `GetPrivateProfileIntW`)
- **Timing**: 10ms timer (`WM_TIMER`) for smooth, low‑latency movement
- **Precision**: Sub‑pixel accumulation – no rounding errors over long bursts

### Algorithm Pseudocode

```
on timer (10ms):
    if active:
        dx = (right - left) / (sensitivity / 10)
        dy = (down - up) / (sensitivity / 10)
        accumX += dx
        accumY += dy
        moveX = floor(accumX)
        moveY = floor(accumY)
        if moveX != 0 or moveY != 0:
            SendInput(moveX, moveY)
            accumX -= moveX
            accumY -= moveY
```

---

## 📝 License

Distributed under the **MIT License**. See `LICENSE` for more information.

---

## 🙏 Acknowledgements

- Microsoft Windows API documentation
- The low‑level hooking community

---

## 📫 Contributions

Pull requests and suggestions are welcome.  
Please open an issue first for major changes.

---

*Made for users who prefer classic, no‑frills utilities with absolute control.*
