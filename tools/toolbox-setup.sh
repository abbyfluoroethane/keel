#!/usr/bin/env bash
# Create the `keel` toolbox and install the build tools into it.
# Safe to run more than once. Never touches the host OS.
set -euo pipefail

NAME=keel
PKGS=(gcc gcc-c++ cmake ninja-build git pkgconf-pkg-config SDL2-devel freetype-devel python3
      python3-pillow)  # Pillow (MIT-CMU): resizes demo cover art in tools/, not linked into Keel

if ! command -v toolbox >/dev/null 2>&1; then
    echo "toolbox not found. Run this on the Fedora Silverblue host." >&2
    exit 1
fi

if [ -f /run/.containerenv ]; then
    echo "Run this script on the host, not inside a container." >&2
    exit 1
fi

# 1. Check if it exists.
if toolbox list --containers 2>/dev/null | awk 'NR > 1 { print $2 }' | grep -qx "$NAME"; then
    echo "toolbox '$NAME' exists"
else
    # 2. Create it.
    echo "creating toolbox '$NAME'"
    toolbox create -y "$NAME"
fi

# 3. Install the build tools. dnf skips packages that are already installed.
toolbox run -c "$NAME" sudo dnf install -y "${PKGS[@]}"

echo
echo "done. build with:"
echo "  toolbox run -c $NAME cmake -B build -G Ninja"
echo "  toolbox run -c $NAME cmake --build build"
echo "  toolbox run -c $NAME ./build/keel"
