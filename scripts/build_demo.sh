#!/usr/bin/env bash
#
# GB2CPC -- build the hello_sprite demo into a runnable CPC .dsk/.cdt.
#
# Generates a CPCtelera project, injects the GB2CPC HAL + demo sources, and
# runs make. Produces build/cpc_demo.dsk (and .cdt) you can load in an emulator
# (RVM, WinAPE, Caprice, Sugarbox) or on real hardware.
#
# Usage:
#   scripts/build_demo.sh
#
# CPCT_PATH is auto-detected from the environment, then from common install
# locations. Override by exporting CPCT_PATH before running.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$SCRIPT_DIR/.." && pwd)"

say() { printf '\n\033[1;36m==> %s\033[0m\n' "$*"; }
die() { printf '\n\033[1;31mERROR: %s\033[0m\n' "$*" >&2; exit 1; }

# --- Locate CPCtelera ------------------------------------------------------
if [ -z "${CPCT_PATH:-}" ]; then
   for c in "$HOME/cpctelera/cpctelera" "$HOME/cpctelera" /opt/cpctelera/cpctelera; do
      if [ -x "$c/tools/scripts/cpct_mkproject" ]; then CPCT_PATH="$c"; break; fi
   done
fi
[ -n "${CPCT_PATH:-}" ] && [ -x "$CPCT_PATH/tools/scripts/cpct_mkproject" ] \
   || die "CPCtelera not found. Run scripts/install_cpctelera_linux.sh first, open a new terminal, or export CPCT_PATH to .../cpctelera."
export CPCT_PATH
say "Using CPCtelera at $CPCT_PATH"

MKPROJECT="$CPCT_PATH/tools/scripts/cpct_mkproject"
BUILD="$REPO/build"
PROJ="$BUILD/cpc_demo"

# --- Create a fresh project skeleton --------------------------------------
say "Creating CPCtelera project at $PROJ"
rm -rf "$PROJ"
mkdir -p "$BUILD"
"$MKPROJECT" "$PROJ" >/dev/null

# --- Inject GB2CPC sources -------------------------------------------------
# build_config.mk puts -Isrc on the include path, so headers placed under
# src/gb2cpc/ resolve as <gb2cpc/...>; every .c under src/ is auto-compiled.
say "Injecting GB2CPC HAL + demo sources"
rm -f "$PROJ"/src/*.c "$PROJ"/src/*.h
mkdir -p "$PROJ/src/gb2cpc"
cp -r "$REPO/include/gb2cpc/." "$PROJ/src/gb2cpc/"
cp "$REPO/src/gb2cpc_core.c"               "$PROJ/src/"
cp "$REPO/examples/hello_sprite/main.c"    "$PROJ/src/"
cp "$REPO/examples/hello_sprite/assets.h"  "$PROJ/src/"

# --- Build -----------------------------------------------------------------
say "Compiling (make)"
( cd "$PROJ" && make )

# --- Collect artifacts -----------------------------------------------------
shopt -s nullglob
artifacts=("$PROJ"/*.dsk "$PROJ"/*.cdt)
[ ${#artifacts[@]} -gt 0 ] || die "Build finished but no .dsk/.cdt produced -- check the make output above."
cp "${artifacts[@]}" "$BUILD/"

say "Success! Artifacts:"
for a in "${artifacts[@]}"; do echo "  $BUILD/$(basename "$a")"; done
cat <<EOF

Run it in an emulator, e.g.:
  - RetroVirtualMachine / Caprice / Sugarbox: open build/cpc_demo.dsk
  - From the CPC prompt:  RUN"CPC_DEMO

Move the sprite with the cursor keys.
EOF
