cmake_minimum_required(VERSION 3.10)
project(openbw-switch CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(OPENBW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/openbw)

include_directories(
    ${OPENBW_DIR}
    ${OPENBW_DIR}/ui
    ${OPENBW_DIR}/deps
)

# SDL2 from devkitPro portlibs (via pkg-config)
find_package(PkgConfig)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

# libnx
set(LIBNX_INCLUDE /opt/devkitpro/libnx/include)
set(LIBNX_LIB /opt/devkitpro/libnx/lib)
include_directories(${LIBNX_INCLUDE})
link_directories(/opt/devkitpro/libnx/lib /opt/devkitpro/portlibs/switch/lib)

add_executable(openbw-switch
    src/main.cpp
    ${OPENBW_DIR}/ui/sdl2.cpp
)

target_compile_options(openbw-switch PRIVATE
    -march=armv8-a+crc+crypto
    -mtune=cortex-a57
    -O2
    -ffunction-sections
    -fdata-sections
    -fPIE
)

target_link_libraries(openbw-switch PRIVATE
    PkgConfig::SDL2
    -Wl,--start-group
    -lSDL2main
    -lSDL2
    -lEGL
    -lglapi
    -ldrm_nouveau
    -lnx
    -lpthread
    -lm
    -Wl,--end-group
)

target_link_options(openbw-switch PRIVATE
    -specs=/opt/devkitpro/libnx/switch.specs
    -fPIE
)

# SDL_MAIN_HANDLED: we provide our own main()
target_compile_definitions(openbw-switch PRIVATE SDL_MAIN_HANDLED OPENBW_NO_SDL_IMAGE OPENBW_NO_SDL_MIXER)
