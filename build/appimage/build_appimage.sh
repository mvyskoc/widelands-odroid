#!/bin/bash
set -e

SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
OLD_PWD=$PWD
trap 'cd "${OLD_PWD}"' EXIT

BUILD_DIR="${SCRIPT_DIR}/compile"
CMAKELIST_PATH="${SCRIPT_DIR}/../.."
INSTALL_DIR="${SCRIPT_DIR}/AppDir"
ARCH=$(uname -m)
APPIMAGE_RECIPE="${SCRIPT_DIR}/AppImageBuilder-${ARCH}.yml"

if [[ "$1" = "clean" ]]; then
    echo "Clean build directory"
    rm -f -r ${INSTALL_DIR}
    rm -f -r ${BUILD_DIR}
    rm -f -r ${appimage-builder-cache}
    rm -f -r "${SCRIPT_DIR}/compile" 
    rm -f "${SCRIPT_DIR}/.flag_cmake" "${SCRIPT_DIR}/*.AppImage"
    exit 0
fi

echo "Install dependency libraries"
sudo apt install liblua5.1-0-dev liboggz2-dev libsdl-net1.2-dev libsdl2-net-dev \
     libsdl-image1.2-dev libsdl-mixer1.2-dev libsdl-ttf2.0-dev libsdl-gfx1.2-dev libsdl-gfx1.2-dev \
     patchelf gettext psmisc

mkdir -p "${BUILD_DIR}"

cd "${BUILD_DIR}"
if [[ -d "${BUILD_DIR}" ]] && [[ -e "${SCRIPT_DIR}/.flag_cmake" ]]; then
    echo "Skip cmake generation, if needed remove .flag_cmake"
else
    rm -f "${SCRIPT_DIR}/.flag_cmake"
    cmake ${CMAKELIST_PATH} -DWL_PORTABLE=false -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DWL_INSTALL_PREFIX=/usr
    touch "${SCRIPT_DIR}/.flag_cmake"
fi

make
make install DESTDIR="${INSTALL_DIR}"

cd "${SCRIPT_DIR}"
cp -v "${SCRIPT_DIR}/data/run.sh" "${INSTALL_DIR}"
chmod +x "${INSTALL_DIR}/run.sh"
mkdir -p "${INSTALL_DIR}/etc/udev/rules.d"
cp -v "${SCRIPT_DIR}"/data/udev/* "${INSTALL_DIR}/etc/udev/rules.d"
mkdir -p "${INSTALL_DIR}/usr/share/games/widelands/xboxdrv"
cp -v "${SCRIPT_DIR}"/data/xboxdrv/* "${INSTALL_DIR}/usr/share/games/widelands/xboxdrv"
cp -v "${SCRIPT_DIR}"/data/kill_widelands.sh "${INSTALL_DIR}/usr/games"
chmod +x "${INSTALL_DIR}/usr/games/kill_widelands.sh"
mkdir -p "${INSTALL_DIR}/usr/share/games/widelands/config"
cp -v "${SCRIPT_DIR}"/data/config-* "${INSTALL_DIR}/usr/share/games/widelands/config"

# Copy icon files 
for res in 32 48 64 128; do
    mkdir -p "${INSTALL_DIR}/usr/share/icons/hicolor/${res}x${res}"
    cp "${CMAKELIST_PATH}/pics/wl-ico-${res}.png" "${INSTALL_DIR}/usr/share/icons/hicolor/${res}x${res}/widelands.png"
done

mkdir -p "${INSTALL_DIR}/usr/share/applications"
echo "${INSTALL_DIR}/usr/share/applications/widelands.desktop"
cat <<EOF > "${INSTALL_DIR}/usr/share/applications/widelands.desktop"
[Desktop Entry]
Type=Application
Name=Widelands
Icon=widelands
Exec=widelands
Categories=Game;
StartupNotify=true
EOF

echo "Build Xboxdrv"
"${CMAKELIST_PATH}/libext/build_xboxdrv.sh"
mkdir -p "${INSTALL_DIR}/usr/bin"
cp "${CMAKELIST_PATH}/libext/xboxdrv/xboxdrv" "${INSTALL_DIR}/usr/bin/" 


./install_appimagekit.sh 
appimage-builder --recipe "${APPIMAGE_RECIPE}" --skip-tests --skip-appimage
appimagetool --comp gzip "${INSTALL_DIR}"

