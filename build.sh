#!/bin/bash

set -e

INFO()  { echo -e "\e[36m[INFO]\e[0m  $1"; }
WARN()  { echo -e "\e[33m[WARN]\e[0m  $1"; }
ERROR() { echo -e "\e[31m[ERROR]\e[0m $1"; }
START() {
    echo ""
    echo "========================================================"
    echo ">> START: $1"
    echo "========================================================"
}
DONE() {
    echo "<< DONE:  $1"
    echo "--------------------------------------------------------"
    echo ""
}

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$ROOT_DIR/build"

if [ -d "$BUILD_DIR" ]; then
    INFO "Clean up old build directory..."
    rm -rf "$BUILD_DIR"
fi

# Clean up stray CMake cache files in build/ directory
if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
    INFO "Removing stray CMakeCache.txt in build/ directory"
    rm -f "$BUILD_DIR/CMakeCache.txt"
fi
if [ -d "$BUILD_DIR/CMakeFiles" ]; then
    INFO "Removing stray CMakeFiles/ in build/ directory"
    rm -rf "$BUILD_DIR/CMakeFiles"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
INFO "build dir: $BUILD_DIR"

CMAKE_OPTIONS=(
    -DPICO_SDK_PATH="$ROOT_DIR/libs/pico-sdk"
    -DPICO_BOARD="pico2_w"
    -DPICO_PLATFORM="rp2350"
    -DFREERTOS_KERNEL_PATH="$ROOT_DIR/libs/FreeRTOS-Kernel"
    -DPICOTOOL_FETCH_FROM_GIT_PATH="$HOME/.pico/picotool"
    )

START "Running CMake configuration"
INFO "CMake Options"
for option in "${CMAKE_OPTIONS[@]}"; do
    INFO "$option"
done
cmake "${CMAKE_OPTIONS[@]}" ..

START "Compiling project"
make -j$(nproc)
if [ $? -eq 0 ]; then
    DONE "Build completed successfully"
else
    ERROR "Build failed"
    exit 1
fi
