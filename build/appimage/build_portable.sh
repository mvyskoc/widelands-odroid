#!/bin/bash
set -e

SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
BUILD_DIR="${SCRIPT_DIR}/compile"
CMAKELIST_PATH="${SCRIPT_DIR}/../.."
INSTALL_DIR="${SCRIPT_DIR}/Widelands"

echo "Install dependency libraries"
sudo apt install liblua5.1-0-dev liboggz2-dev libsdl-net1.2-dev libsdl2-net-dev \
     libsdl-image1.2-dev libsdl-mixer1.2-dev libsdl-ttf2.0-dev libsdl-gfx1.2-dev libsdl-gfx1.2-dev \
     patchelf gettext

mkdir -p "${BUILD_DIR}"

cd "${BUILD_DIR}"
cmake -DWL_PORTABLE=true ${CMAKELIST_PATH} -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}"
make
make lang
make install

cp ${SCRIPT_DIR}/data/* ${INSTALL_DIR}

