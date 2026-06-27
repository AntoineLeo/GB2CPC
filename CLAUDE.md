# GB2CPC -- project notes for Claude Code

Game Boy -> Amstrad CPC compatibility layer & toolchain. See `README.md` for
the vision and `docs/BUILD.md` for how to build.

## Layout
- `tools/` -- Python asset pipeline (dependency-free, standard library only).
  - `asset_converter.py` -- CLI entry point.
  - `gb2cpc/tiles.py` -- GB 2BPP -> CPC Mode 1 packing.
  - `gb2cpc/palette.py` -- GB 4-shade -> CPC 27-ink mapping.
  - `gb2cpc/emit.py` -- C header generation.
- `include/gb2cpc/` -- GBDK-compatible headers (`gb/gb.h`, `gb/hardware.h`,
  `types.h`). Decompiled GB / GBDK sources include these unchanged.
- `src/gb2cpc_core.c` -- runtime implementation on top of CPCtelera (virtual
  OAM, software sprite manager, double-buffered render, keyboard->joypad).
- `examples/hello_sprite/` -- PoC: a sprite moved with the d-pad.
- `tests/test_pipeline.py` -- pipeline unit tests (runnable with plain Python).

## Conventions
- Keep the Python pipeline standard-library only (runs in any build step).
- Header API names mirror GBDK exactly so GB sources need no edits; new,
  CPC-specific entry points are prefixed `gb2cpc_`.
- CPC target is Mode 1 (320x200, 4 colours); GB 160x144 viewport is centred.
- The C runtime targets SDCC + CPCtelera and is not compilable in this repo
  without that toolchain installed.

## Build / test
- `make test` -- run pipeline tests (works anywhere Python is present).
- `make assets` -- regenerate the demo sprite header.
- `make syntax-check CPCT_INCLUDE=...` -- SDCC compile-check of the HAL.

## Current status: Early Alpha
Working: asset pipeline (tested). Pending first on-hardware build: the C
runtime. Optimisation TODO (per README): dirty-rectangle erase-and-redraw
instead of the current full-frame redraw, and runtime sprite X/Y flipping.
