#!/usr/bin/env sh
set -eu

SETUP_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

if command -v python3 >/dev/null 2>&1; then
  exec python3 "$SETUP_DIR/setup.py" "$@"
fi

exec python "$SETUP_DIR/setup.py" "$@"
