# Building GB2CPC

GB2CPC has two halves with different toolchain needs.

## 1. Asset pipeline (Python only)

No retro toolchain required. Python 3.8+ is enough (standard library only).

```bash
# Run the unit tests
python tests/test_pipeline.py        # or: make test

# Preview the GB->CPC palette mapping
python tools/asset_converter.py --palette-only

# Convert raw Game Boy 2BPP tile data into a CPC Mode 1 C header
python tools/asset_converter.py -i graphics.2bpp -o src/assets.h --name bkg_tiles
```

If you start from PNG images, produce the 2BPP stream upstream with RGBDS:

```bash
rgbgfx -o graphics.2bpp graphics.png
```

## 2. Runtime library + demo (SDCC + CPCtelera)

The compatibility layer compiles for the Z80 with **SDCC** and links against
**CPCtelera** for the low-level CPC routines.

### Prerequisites
- [CPCtelera](https://github.com/lronaldo/cpctelera) installed and on `PATH`
  (provides `<cpctelera.h>`, the build system, and packaging tools).
- SDCC (bundled with / required by CPCtelera).

### Fastest path: native Linux (scripted)
On a native Linux machine the two helper scripts do everything end to end:

```bash
# 1. Install CPCtelera (deps via your package manager + compiles SDCC).
#    Needs sudo and ~20-40 min for the SDCC build. Clones to ~/cpctelera.
scripts/install_cpctelera_linux.sh

# 2. Open a new terminal (so CPCT_PATH is on your PATH), then:
scripts/build_demo.sh
```

`build_demo.sh` generates a CPCtelera project under `build/cpc_demo/`, injects
the GB2CPC HAL + demo (headers go to `src/gb2cpc/`, resolved via `-Isrc`), runs
`make`, and copies `build/cpc_demo.dsk` / `.cdt` to `build/`. Load the `.dsk`
in an emulator (RVM, Caprice, Sugarbox, WinAPE) and `RUN"CPC_DEMO`.

> WSL note: the same scripts work in WSL, but install CPCtelera inside the WSL
> native filesystem (the default `~/cpctelera`), never under `/mnt/c`, or the
> SDCC compile is painfully slow.

### Manual path: build inside a CPCtelera project
If you prefer to wire it up by hand (this is what `build_demo.sh` automates):

1. Create a project skeleton: `cpct_mkproject hello_gb2cpc`
2. Copy `include/gb2cpc` into the project's `src/` (as `src/gb2cpc/`), then copy
   `src/gb2cpc_core.c` and `examples/hello_sprite/{main.c,assets.h}` into `src/`.
   Everything under `src/` is auto-compiled and `-Isrc` is already on the
   include path, so `<gb2cpc/gb/gb.h>` resolves with no extra config.
3. Keep code/data below `0x8000`: `gb2cpc_core.c` uses `0x8000` and `0xC000` as
   its double-buffer pages. The default `Z80CODELOC` (`0x4000`) leaves ~16 KB,
   ample for the demo.
4. `make` -> the resulting `.dsk` runs on a CPC 6128 or emulator.

### Quick syntax check (no linking)
To catch C-level errors without a full build:

```bash
make syntax-check CPCT_INCLUDE=/path/to/cpctelera/cpctelera/src
```

This compiles `gb2cpc_core.c` to an object file only; it does not link a CPC
binary.

## Status

This is **Early Alpha**. The Python pipeline is tested and working. The C
runtime is written to the documented CPCtelera API but awaits its first
on-hardware integration build (see TODOs in `src/gb2cpc_core.c`).
