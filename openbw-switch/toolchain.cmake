# Cross-compile toolchain for Nintendo Switch (devkitA64)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

if(DEFINED ENV{DEVKITPRO})
	set(DEVKITPRO $ENV{DEVKITPRO})
else()
	set(DEVKITPRO /opt/devkitpro)
endif()
set(DEVKITA64 ${DEVKITPRO}/devkitA64)

set(CMAKE_C_COMPILER ${DEVKITA64}/bin/aarch64-none-elf-gcc)
set(CMAKE_CXX_COMPILER ${DEVKITA64}/bin/aarch64-none-elf-g++)

set(CMAKE_FIND_ROOT_PATH
    ${DEVKITA64}
    ${DEVKITPRO}/libnx
    ${DEVKITPRO}/portlibs/switch
)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(ENV{PKG_CONFIG_PATH} ${DEVKITPRO}/portlibs/switch/lib/pkgconfig)
set(CMAKE_PREFIX_PATH ${DEVKITPRO}/portlibs/switch)
