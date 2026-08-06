#!/usr/bin/env bash
# Build a .deb package from a staged directory.
#
# Usage: build-deb.sh <stage_dir> <output.deb>
#
# Uses dpkg-deb when available (Debian/Ubuntu). Falls back to a
# self-contained Python packer so the package can be built anywhere.

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
STAGE=$1
OUT=$2

if [[ ! -d "$STAGE/DEBIAN" ]]; then
    echo "error: $STAGE is not a staged package (missing DEBIAN/)" >&2
    exit 1
fi

mkdir -p "$(dirname "$OUT")"
OUT=$(cd "$(dirname "$OUT")" && pwd)/$(basename "$OUT")

if command -v dpkg-deb >/dev/null 2>&1; then
    echo "==> Building $OUT with dpkg-deb"
    dpkg-deb --root-owner-group --build "$STAGE" "$OUT"
else
    echo "==> dpkg-deb not found, building $OUT with Python fallback packer"
    python3 "$SCRIPT_DIR/build-deb.py" "$STAGE" "$OUT"
fi
