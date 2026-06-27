"""Game Boy 2BPP tiles  ->  Amstrad CPC Mode 1 linear bytes.

Game Boy tile format (2BPP, planar)
-----------------------------------
An 8x8 tile is 16 bytes: 2 bytes per row. For row ``r`` byte ``2r`` is the LSB
plane and byte ``2r+1`` the MSB plane. For pixel column ``c`` (0 = leftmost,
read from bit 7 down to bit 0)::

    low  = (plane0 >> (7 - c)) & 1
    high = (plane1 >> (7 - c)) & 1
    pixel = (high << 1) | low          # 0..3, index into the GB palette

Amstrad CPC Mode 1 format (2BPP, packed)
----------------------------------------
Mode 1 packs 4 pixels per byte. For the 4 pixels p0..p3 (left to right) with
pixel value bits b0 (LSB) and b1 (MSB)::

    bit7 = p0.b0   bit6 = p1.b0   bit5 = p2.b0   bit4 = p3.b0
    bit3 = p0.b1   bit2 = p1.b1   bit1 = p2.b1   bit0 = p3.b1

So an 8-pixel row becomes 2 bytes and a full 8x8 tile becomes 16 bytes, the
exact byte budget CPCtelera sprites expect for an 8x8 Mode 1 sprite.
"""

from __future__ import annotations

from typing import List, Sequence

GB_TILE_SIZE_BYTES = 16  # 8x8 @ 2bpp planar
CPC_MODE1_TILE_BYTES = 16  # 8 rows * 2 bytes/row


def decode_gb_tile(data: Sequence[int], offset: int = 0) -> List[List[int]]:
    """Decode one 16-byte GB tile into an 8x8 grid of pixel values (0..3)."""
    if offset + GB_TILE_SIZE_BYTES > len(data):
        raise ValueError("not enough bytes for a full GB tile")
    rows: List[List[int]] = []
    for r in range(8):
        plane0 = data[offset + r * 2]
        plane1 = data[offset + r * 2 + 1]
        row: List[int] = []
        for c in range(8):
            low = (plane0 >> (7 - c)) & 1
            high = (plane1 >> (7 - c)) & 1
            row.append((high << 1) | low)
        rows.append(row)
    return rows


def _encode_mode1_byte(p0: int, p1: int, p2: int, p3: int) -> int:
    """Pack 4 pixel values (0..3) into one CPC Mode 1 byte."""
    pixels = (p0, p1, p2, p3)
    byte = 0
    for i, px in enumerate(pixels):
        b0 = px & 1
        b1 = (px >> 1) & 1
        byte |= b0 << (7 - i)
        byte |= b1 << (3 - i)
    return byte


def encode_cpc_mode1_tile(grid: Sequence[Sequence[int]], remap: Sequence[int] | None = None) -> List[int]:
    """Encode an 8x8 pixel grid into 16 CPC Mode 1 bytes.

    ``remap`` optionally translates GB pixel value -> CPC pen (length 4).
    """
    if len(grid) != 8 or any(len(row) != 8 for row in grid):
        raise ValueError("tile grid must be 8x8")
    out: List[int] = []
    for row in grid:
        px = [remap[v] if remap else v for v in row]
        out.append(_encode_mode1_byte(px[0], px[1], px[2], px[3]))
        out.append(_encode_mode1_byte(px[4], px[5], px[6], px[7]))
    return out


def convert_tileset(data: Sequence[int], remap: Sequence[int] | None = None) -> List[List[int]]:
    """Convert a buffer of concatenated GB tiles into a list of CPC tiles.

    Returns one 16-byte list per tile. Raises if ``data`` is not a whole
    number of GB tiles.
    """
    if len(data) % GB_TILE_SIZE_BYTES != 0:
        raise ValueError(
            f"input length {len(data)} is not a multiple of {GB_TILE_SIZE_BYTES} (one GB tile)"
        )
    n_tiles = len(data) // GB_TILE_SIZE_BYTES
    tiles: List[List[int]] = []
    for t in range(n_tiles):
        grid = decode_gb_tile(data, t * GB_TILE_SIZE_BYTES)
        tiles.append(encode_cpc_mode1_tile(grid, remap))
    return tiles
