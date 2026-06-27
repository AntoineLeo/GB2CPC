"""GB2CPC asset pipeline.

Offline converters that translate Game Boy graphics data into the formats
consumed by the GB2CPC runtime on the Amstrad CPC:

* :mod:`gb2cpc.tiles`   -- Game Boy 2BPP tiles  ->  CPC Mode 1 linear bytes.
* :mod:`gb2cpc.palette` -- Game Boy 4-shade palette -> CPC hardware inks.

The package is intentionally dependency-free (pure standard library) so it can
run inside any build step without a virtualenv.
"""

__version__ = "0.1.0"

from .palette import (  # noqa: F401
    CPC_FIRMWARE_INKS,
    GB_DMG_SHADES,
    PaletteMapping,
    map_gb_palette,
)
from .tiles import (  # noqa: F401
    GB_TILE_SIZE_BYTES,
    convert_tileset,
    decode_gb_tile,
    encode_cpc_mode1_tile,
)
