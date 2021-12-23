#!/bin/bash
set -e
OLD_PWD=$PWD
trap 'cd "${OLD_PWD}"' EXIT

SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
XBOXDRV="${SCRIPT_DIR}/xboxdrv"

cd "${XBOXDRV}"
if [[ ! -e  "${XBOXDRV}/.patched" ]]; then
    git am ./../patches/xboxdrv/*.patch
    touch "${XBOXDRV}/.patched"
else
    echo "Already patched - skipped"
fi

sudo apt-get install \
 g++ \
 libboost1.71-dev \
 scons \
 pkg-config \
 libusb-1.0-0-dev \
 libx11-dev \
 libudev-dev \
 x11proto-core-dev \
 libdbus-glib-1-dev
 
python2 `which scons`

