"""Unit tests for the GB2CPC asset pipeline.

Run with:  python -m pytest tests/        (if pytest is installed)
       or:  python tests/test_pipeline.py  (zero-dependency self-check)
"""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))

from gb2cpc.palette import map_gb_palette, nearest_cpc_ink  # noqa: E402
from gb2cpc.tiles import (  # noqa: E402
    convert_tileset,
    decode_gb_tile,
    encode_cpc_mode1_tile,
)


def test_decode_solid_tiles():
    # All bytes 0x00 -> every pixel value 0.
    grid = decode_gb_tile([0x00] * 16)
    assert all(v == 0 for row in grid for v in row)

    # plane0=0xFF, plane1=0x00 on each row -> pixel value 1 everywhere.
    data = []
    for _ in range(8):
        data += [0xFF, 0x00]
    grid = decode_gb_tile(data)
    assert all(v == 1 for row in grid for v in row)

    # both planes 0xFF -> pixel value 3 everywhere.
    data = [0xFF, 0xFF] * 8
    grid = decode_gb_tile(data)
    assert all(v == 3 for row in grid for v in row)


def test_decode_pixel_order():
    # plane0 = 0b10000000 (bit7) -> leftmost pixel low bit set, value 1.
    grid = decode_gb_tile([0x80, 0x00] + [0x00] * 14)
    assert grid[0][0] == 1
    assert all(grid[0][c] == 0 for c in range(1, 8))


def test_mode1_byte_packing():
    # Pixels (3,0,0,0): p0 b0->bit7, p0 b1->bit3 => 0b10001000 = 0x88
    b = encode_cpc_mode1_tile([[3, 0, 0, 0, 0, 0, 0, 0]] + [[0] * 8] * 7)[0]
    assert b == 0x88
    # Pixels (0,0,0,3): p3 b0->bit4, p3 b1->bit0 => 0b00010001 = 0x11
    b = encode_cpc_mode1_tile([[0, 0, 0, 3, 0, 0, 0, 0]] + [[0] * 8] * 7)[0]
    assert b == 0x11


def test_convert_tileset_sizes():
    data = [0xFF, 0xFF] * 8 * 3  # 3 tiles
    tiles = convert_tileset(data)
    assert len(tiles) == 3
    assert all(len(t) == 16 for t in tiles)
    # value 3 everywhere => every Mode 1 byte has all 4 pixels = 3 => 0xFF
    assert all(b == 0xFF for t in tiles for b in t)


def test_convert_tileset_rejects_partial():
    try:
        convert_tileset([0x00] * 15)
    except ValueError:
        return
    raise AssertionError("expected ValueError for partial tile")


def test_palette_mapping_distinct():
    pm = map_gb_palette()
    assert len(pm.inks) == 4
    assert len(set(pm.inks)) == 4, "the 4 GB shades must map to 4 distinct CPC inks"
    # darkest shade should map to (near) black ink 12.
    assert nearest_cpc_ink((0x08, 0x18, 0x20)) == 12


def _run_all():
    fns = [v for k, v in sorted(globals().items()) if k.startswith("test_") and callable(v)]
    failures = 0
    for fn in fns:
        try:
            fn()
            print(f"PASS {fn.__name__}")
        except Exception as exc:  # noqa: BLE001
            failures += 1
            print(f"FAIL {fn.__name__}: {exc}")
    print(f"\n{len(fns) - failures}/{len(fns)} passed")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(_run_all())
