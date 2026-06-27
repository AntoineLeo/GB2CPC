#!/usr/bin/env python3
"""GB2CPC asset converter -- CLI entry point for the asset pipeline.

Converts Game Boy graphics into a C header consumable by the GB2CPC runtime.

Examples
--------
Convert raw 2BPP tile data (the typical output of a GB decompilation) into a
Mode 1 header::

    python tools/asset_converter.py --input graphics.2bpp --mode 1 \\
        --output src/assets.h --name bkg_tiles

Print only the chosen CPC palette for the default DMG shades::

    python tools/asset_converter.py --palette-only

Notes
-----
The ``.gbr`` (Game Boy Tile Designer) container shown in the README is a
project format; this tool consumes the raw 2BPP tile stream that decompiled
projects and ``rgbgfx`` emit. Use ``rgbgfx -o out.2bpp in.png`` upstream if you
start from images.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

# Allow running both as "python tools/asset_converter.py" and as a module.
sys.path.insert(0, str(Path(__file__).resolve().parent))

from gb2cpc.emit import emit_header  # noqa: E402
from gb2cpc.palette import map_gb_palette  # noqa: E402
from gb2cpc.tiles import convert_tileset  # noqa: E402


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="asset_converter",
        description="Convert Game Boy graphics to Amstrad CPC formats.",
    )
    p.add_argument("--input", "-i", help="Input raw 2BPP tile file (Game Boy format).")
    p.add_argument("--output", "-o", help="Output C header path. Defaults to stdout.")
    p.add_argument(
        "--mode",
        type=int,
        default=1,
        choices=[1],
        help="Target CPC screen mode (only Mode 1 supported in alpha).",
    )
    p.add_argument(
        "--name",
        default="gb2cpc_tiles",
        help="Base name for the generated C array/macros.",
    )
    p.add_argument(
        "--no-palette-remap",
        action="store_true",
        help="Keep GB pixel values 0..3 as-is instead of remapping to CPC pens.",
    )
    p.add_argument(
        "--palette-only",
        action="store_true",
        help="Print the computed CPC palette and exit (no tile input needed).",
    )
    return p


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)

    palette = map_gb_palette()

    if args.palette_only:
        print("GB shade -> CPC hardware ink:")
        for shade, ink, rgb in zip(range(4), palette.inks, palette.rgb):
            print(f"  shade {shade}: ink {ink:2d}  rgb#{rgb[0]:02X}{rgb[1]:02X}{rgb[2]:02X}")
        print()
        print(palette.as_c_array(args.name + "_palette"), end="")
        return 0

    if not args.input:
        build_parser().error("--input is required unless --palette-only is given")

    data = Path(args.input).read_bytes()
    # Identity remap keeps pen indices equal to GB shade indices (0..3), which
    # the runtime then resolves through the hardware palette set via
    # cpct_setPalette(gb2cpc_tiles_palette). So "remap" here is identity by
    # design; --no-palette-remap is kept for parity / future modes.
    remap = None if args.no_palette_remap else [0, 1, 2, 3]
    tiles = convert_tileset(data, remap)

    header = emit_header(tiles, args.name, palette)

    if args.output:
        out = Path(args.output)
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(header)
        print(
            f"Wrote {len(tiles)} tile(s) -> {out} "
            f"({len(tiles) * 16} bytes of Mode 1 data)",
            file=sys.stderr,
        )
    else:
        sys.stdout.write(header)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
