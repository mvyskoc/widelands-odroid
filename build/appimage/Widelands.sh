#!/bin/bash
SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

APPIMAGE=("${SCRIPT_DIR}"/Widelands-*.AppImage)

mkdir -p "${APPIMAGE}.home"
mkdir -p "${APPIMAGE}.config"

"$APPIMAGE" --setup $@

