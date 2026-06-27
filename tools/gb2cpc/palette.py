"""Game Boy palette  ->  Amstrad CPC palette mapping.

The original DMG Game Boy renders 4 shades of olive-green grey. The Amstrad CPC
exposes a fixed hardware palette of 27 colours, of which Mode 1 can display 4 at
a time. ``gb2cpc-palette`` picks, for each of the 4 Game Boy shades, the nearest
CPC hardware ink (by Euclidean distance in linear RGB).

CPC colours are referenced by their *hardware* number (the value written to the
Gate Array, 0..26), which is what CPCtelera's ``cpct_setPalette`` expects.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import List, Sequence, Tuple

RGB = Tuple[int, int, int]

# --- Reference colours -------------------------------------------------------

# Classic DMG 4-shade palette as approximate sRGB, shade 0 (lightest) -> 3
# (darkest). This matches how decompiled GB projects label shades 0..3.
GB_DMG_SHADES: Tuple[RGB, RGB, RGB, RGB] = (
    (0xE0, 0xF8, 0xD0),  # 0 - lightest
    (0x88, 0xC0, 0x70),  # 1 - light
    (0x34, 0x68, 0x56),  # 2 - dark
    (0x08, 0x18, 0x20),  # 3 - darkest / black
)

# The 27 CPC hardware inks. The Gate Array uses 3 levels per channel
# (0x00, 0x80, 0xFF). Index == hardware colour number expected by the GA.
# Order follows the canonical CPCtelera / Gate Array hardware numbering.
CPC_FIRMWARE_INKS: Tuple[RGB, ...] = (
    (0x80, 0x80, 0x80),  # 0  white (mid)        -> GA 0x54
    (0x00, 0x00, 0x80),  # 1
    (0x00, 0xFF, 0x80),  # 2
    (0xFF, 0x00, 0x80),  # 3
    (0x00, 0x00, 0x80),  # 4
    (0xFF, 0x00, 0x80),  # 5
    (0x80, 0xFF, 0x80),  # 6
    (0xFF, 0xFF, 0x80),  # 7
    (0xFF, 0x00, 0x80),  # 8
    (0xFF, 0xFF, 0x80),  # 9
    (0xFF, 0xFF, 0x00),  # 10
    (0xFF, 0xFF, 0xFF),  # 11
    (0x00, 0x00, 0x00),  # 12 black
    (0x00, 0x00, 0xFF),  # 13 bright blue
    (0x00, 0xFF, 0x00),  # 14 bright green
    (0x00, 0xFF, 0xFF),  # 15 bright cyan
    (0xFF, 0x00, 0x00),  # 16 bright red
    (0xFF, 0x00, 0xFF),  # 17 bright magenta
    (0x80, 0x80, 0x00),  # 18
    (0x80, 0x80, 0xFF),  # 19
    (0x00, 0x80, 0x00),  # 20
    (0x00, 0x80, 0xFF),  # 21
    (0x80, 0x00, 0x00),  # 22
    (0x80, 0x00, 0xFF),  # 23
    (0x80, 0x80, 0x00),  # 24
    (0xFF, 0x80, 0x00),  # 25
    (0x00, 0x80, 0x80),  # 26
)


@dataclass
class PaletteMapping:
    """Result of mapping the 4 GB shades onto CPC hardware inks."""

    #: CPC hardware ink number for each GB shade (length 4, shade 0..3).
    inks: List[int]
    #: The RGB actually chosen on the CPC side (for previewing / debugging).
    rgb: List[RGB]

    def as_c_array(self, name: str = "gb2cpc_palette") -> str:
        """Emit a C ``const`` array usable with ``cpct_setPalette``."""
        body = ", ".join(str(i) for i in self.inks)
        return f"const unsigned char {name}[4] = {{ {body} }};\n"


def _distance2(a: RGB, b: RGB) -> int:
    return (a[0] - b[0]) ** 2 + (a[1] - b[1]) ** 2 + (a[2] - b[2]) ** 2


def nearest_cpc_ink(rgb: RGB) -> int:
    """Return the CPC hardware ink number nearest to ``rgb``."""
    best_idx, best_d = 0, None
    for idx, ink in enumerate(CPC_FIRMWARE_INKS):
        d = _distance2(rgb, ink)
        if best_d is None or d < best_d:
            best_idx, best_d = idx, d
    return best_idx


def map_gb_palette(shades: Sequence[RGB] = GB_DMG_SHADES) -> PaletteMapping:
    """Map the 4 GB shades to the nearest available CPC hardware inks.

    Duplicate inks are nudged to the next-nearest free ink so the 4 GB shades
    always map to 4 *distinct* CPC pens, preserving contrast.
    """
    if len(shades) != 4:
        raise ValueError("Game Boy palette must contain exactly 4 shades")

    inks: List[int] = []
    used: set[int] = set()
    for rgb in shades:
        # Rank inks by distance and take the first not already used.
        ranked = sorted(
            range(len(CPC_FIRMWARE_INKS)),
            key=lambda idx: _distance2(rgb, CPC_FIRMWARE_INKS[idx]),
        )
        chosen = next((i for i in ranked if i not in used), ranked[0])
        used.add(chosen)
        inks.append(chosen)

    return PaletteMapping(inks=inks, rgb=[CPC_FIRMWARE_INKS[i] for i in inks])
