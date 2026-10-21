#!/bin/bash
# Build StarCraft (OpenBW engine) for Nintendo Switch -> .nro
# Requirements: devkitA64 + libnx + switch-sdl2 installed under /opt/devkitpro
set -e
cd "$(dirname "$0")"

export DEVKITPRO=/opt/devkitpro
export DEVKITA64=$DEVKITPRO/devkitA64
export PATH=$DEVKITA64/bin:/opt/devkitpro/tools/bin:$PATH
export PKG_CONFIG_PATH=$DEVKITPRO/portlibs/switch/lib/pkgconfig

BUILD=build
rm -rf $BUILD
mkdir -p $BUILD
cd $BUILD

cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake
make -j$(nproc)

elf2nro openbw-switch openbw-switch.nro --icon=$DEVKITPRO/libnx/default_icon.jpg

ls -la openbw-switch.nro
echo "DONE: build/openbw-switch.nro  (copy to sdmc:/switch/openbw-switch.nro)"
