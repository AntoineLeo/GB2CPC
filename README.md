# GB2CPC: Game Boy to Amstrad CPC Compatibility Layer & Toolchain

**GB2CPC** is an open-source, compile-time hardware abstraction layer (HAL) and toolchain designed to ease the porting of decompiled Game Boy games (or modern GBDK projects) to the Amstrad CPC.

By bridging the gap between the Game Boy's Sharp LR35902 / PPU architecture and the Amstrad CPC's Zilog Z80 / CRTC bitmap system, this project aims to provide a set of tools and C libraries to recompile Game Boy codebases targeting the Amstrad CPC (using **CPCtelera** or **SDCC**).

---

## 🚀 The Vision

With the rise of static decompilation projects (like `gb-recompiled`), we now have C source code for legendary Game Boy titles. Since both machines share an almost identical CPU core and logic, **the game logic itself is portable**. 

The missing link is the hardware. **GB2CPC** acts as a translation layer: it intercepts Game Boy hardware registry writes and GBDK API calls, mapping them into highly optimized Amstrad CPC drawing and system routines.


```

[ Decompiled GB C Code ]
│
├──► (Calls standard GBDK functions or writes to GB VRAM)
│
[ GB2CPC Wrapper Layer ] ──► (Interceptors / Macros / Optimized Z80 Assemblies)
│
├──► (Translates Tile/Sprite logic into CPC Software Sprites)
│
[ Amstrad CPC Binary ] (Targeting 64K/128K CRTC Bitmap)

```

---

## 🛠️ Repository Structure & Component Roadmap

The project is split into two major parts: **Offline Build Tools** (Asset Converters) and the **Runtime Library** (The Wrapper).

### 1. `/tools` (Asset Pipeline)
A Python-based asset converter that hooks into the build process.
* **`gb2cpc-tiles`**: Converts Game Boy 2BPP (4 shades of gray) tilesets/backgrounds into Amstrad CPC Mode 1 (4 colors, interleaved memory) linear sprite arrays.
* **`gb2cpc-palette`**: Maps the Game Boy's 4-shade palette to the closest hardware colors available in the Amstrad's 27-color palette.

### 2. `/include` & `/src` (The Compatibility Layer)
The core C header files that mimic Game Boy registers and common **GBDK (Game Boy Development Kit)** APIs.
* **`gb/hardware.h`**: Simulates GB Memory Mapped I/O (`rLCDC`, `rSTAT`, `rLY`, etc.).
* **`gb/gb.h`**: Implements classic functions like `set_sprite_tile()`, `move_sprite()`, `set_bkg_data()`, and `joypad()`.

---

## 🧠 Technical Architecture & Challenges

### 📺 Graphics: Tile/Sprite hardware vs. Bitmap buffer
The Game Boy handles tiles and sprites natively via its PPU hardware. The Amstrad CPC is a pure bitmap machine.
* **Our Approach**: GB2CPC utilizes a **Software Sprite Manager** (leveraging *CPCtelera* low-level assembly routines). When the Game Boy code updates the virtual OAM (Object Attribute Memory), GB2CPC schedules an erase-and-redraw cycle on the CPC's back-buffer to achieve flicker-free 25/50 Hz rendering.
* **Display Target**: By default, the wrapper targets **CPC Mode 1 (320x200, 4 colors)**, rendering a 160x144 viewport centered or scaled, preserving pixel aspect ratio perfectly.

### 💾 Memory & Bank Switching (MBC)
Large Game Boy games use Memory Bank Controllers (MBC) to swap ROM/RAM banks.
* **Our Approach**: For games exceeding ~48KB, GB2CPC provides conditional compiling options to leverage the **Amstrad CPC 128K page switching** mechanism (&4000-&7FFF bank swapping), making it fully compatible with the CPC 6128 footprint.

### 🕹️ Input Mapping
Game Boy joypad register bits are mapped transparently onto the Amstrad CPC Keyboard Matrix and Port 1 Joystick via aggressive inline macros to prevent CPU overhead.

---

## 🏁 Getting Started (Proof of Concept)

Currently, the project is in **Early Alpha**. We are benchmarking a basic GBDK "Hello World" (a moving sprite with joypad inputs) compiled for the CPC.

### Prerequisites
* **CPCtelera** setup.
* **Python 3.x** (for the asset pipeline).

### Compilation Pipeline (Concept)
To compile a Game Boy source file for the CPC using this wrapper:

```bash
# 1. Convert Game Boy assets to CPC format
python3 tools/asset_converter.py --input graphics.gbr --mode 1 --output src/assets.h

# 2. Compile using CPCtelera/SDCC while forcing our local GB include path
sdcc -mz80 -I./include/gb2cpc -I./src main.c gb2cpc_core.c -o build/game.ihx

```

---

## 🤝 Contributing

We are looking for retro-developers, assembly enthusiasts, and Amstrad/Game Boy scene experts!

Areas where help is critically needed:

* [ ] **Z80 Assembly Optimization:** Making the software sprite engine fast enough to handle 10+ concurrent virtual sprites on a 4MHz Z80.
* [ ] **Audio Translation:** Building a converter or runtime translator from the Game Boy's 4-channel sound registers to the CPC's AY-3-8912 chip.
* [ ] **Testing:** Porting simple, open-source GBDK homebrews to stress-test the API coverage.

---

## 📜 License

This project is licensed under the MIT License - see the [LICENSE](https://www.google.com/search?q=LICENSE) file for details.

*Disclaimer: GB2CPC is a community-driven hobbyist project. It is not affiliated with, authorized, or endorsed by Nintendo or Amstrad.*

```
