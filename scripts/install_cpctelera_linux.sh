#!/usr/bin/env bash
#
# GB2CPC -- one-shot CPCtelera installer for native Linux.
#
# Installs the system dependencies CPCtelera needs, clones it, and runs its
# setup.sh (which compiles SDCC from source + the CPCtelera Z80 library and
# registers CPCT_PATH in your shell profile).
#
# Usage:
#   scripts/install_cpctelera_linux.sh [install_dir]
#
#   install_dir   Where to clone CPCtelera. Default: $HOME/cpctelera
#                 (use a native Linux filesystem -- NOT an NTFS/9p mount, the
#                 SDCC build is slow and fragile there).
#
# After it finishes, open a new terminal (or `source ~/.bashrc`) so CPCT_PATH
# and the tools land on your PATH, then run scripts/build_demo.sh.
set -euo pipefail

INSTALL_DIR="${1:-$HOME/cpctelera}"
REPO_URL="https://github.com/lronaldo/cpctelera.git"

say() { printf '\n\033[1;36m==> %s\033[0m\n' "$*"; }
die() { printf '\n\033[1;31mERROR: %s\033[0m\n' "$*" >&2; exit 1; }

# --- 1. System dependencies ------------------------------------------------
install_deps() {
   say "Installing build dependencies (sudo required)"
   if command -v apt-get >/dev/null; then
      sudo apt-get update
      sudo apt-get install -y \
         build-essential bison flex texinfo gettext git curl \
         libboost-dev libfreeimage-dev mono-complete
   elif command -v dnf >/dev/null; then
      sudo dnf install -y \
         gcc gcc-c++ make bison flex texinfo gettext-devel git curl \
         boost-devel freeimage-devel mono-complete
   elif command -v pacman >/dev/null; then
      sudo pacman -S --needed --noconfirm \
         base-devel bison flex texinfo gettext git curl \
         boost freeimage mono
   elif command -v zypper >/dev/null; then
      sudo zypper install -y \
         -t pattern devel_basis
      sudo zypper install -y \
         bison flex texinfo gettext-tools git curl \
         libboost_headers-devel freeimage-devel mono-complete
   else
      die "No supported package manager found (apt/dnf/pacman/zypper). Install these manually: gcc g++ make bison flex texinfo gettext libboost-dev libfreeimage-dev mono, then re-run."
   fi
}

# --- 2. Clone CPCtelera ----------------------------------------------------
clone_cpctelera() {
   if [ -d "$INSTALL_DIR/.git" ]; then
      say "CPCtelera already cloned at $INSTALL_DIR -- pulling latest"
      git -C "$INSTALL_DIR" pull --ff-only || true
   else
      say "Cloning CPCtelera into $INSTALL_DIR"
      git clone --depth 1 "$REPO_URL" "$INSTALL_DIR"
   fi
}

# --- 3. Run setup.sh (compiles SDCC + library) -----------------------------
run_setup() {
   say "Running CPCtelera setup.sh (this compiles SDCC -- expect 20-40 min)"
   ( cd "$INSTALL_DIR" && ./setup.sh -dme )
}

main() {
   case "$(uname -s)" in
      Linux) : ;;
      *) die "This script targets native Linux. On macOS use CPCtelera's own setup; on Windows use WSL." ;;
   esac

   install_deps
   clone_cpctelera
   run_setup

   say "Done."
   cat <<EOF

CPCtelera installed at: $INSTALL_DIR/cpctelera
CPCT_PATH was added to your shell profile by setup.sh.

Next steps:
  1) Open a NEW terminal (or run: source ~/.bashrc)
  2) Verify:   echo \$CPCT_PATH    # should print $INSTALL_DIR/cpctelera
  3) Build the demo:  scripts/build_demo.sh

EOF
}

main "$@"
