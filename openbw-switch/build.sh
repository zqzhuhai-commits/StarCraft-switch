#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
export DEVKITPRO="${DEVKITPRO:-/opt/devkitpro}"
export DEVKITA64="${DEVKITA64:-$DEVKITPRO/devkitA64}"
export PATH="$DEVKITA64/bin:$DEVKITPRO/tools/bin:$PATH"
export PKG_CONFIG_LIBDIR="$DEVKITPRO/portlibs/switch/lib/pkgconfig"
export PKG_CONFIG_PATH="$PKG_CONFIG_LIBDIR"
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake
cmake --build build --parallel "${JOBS:-2}"
nacptool --create "星际争霸" "StarCraft Switch Community" "0.2.0" build/StarCraft-switch.nacp
elf2nro build/openbw-switch build/StarCraft-switch.nro --icon=icon.jpg --nacp=build/StarCraft-switch.nacp
elf2nso build/openbw-switch build/StarCraft-switch.nso
printf 'Built NRO and NSO in %s/build\n' "$PWD"
