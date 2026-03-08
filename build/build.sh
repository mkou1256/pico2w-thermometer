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

BUILD_DIR="$(cd "$(dirname "$0")" && pwd)"
OUT_DIR="$BUILD_DIR/out"
PROJECT_ROOT_DIR="$BUILD_DIR/.."

if [ -d "$OUT_DIR" ]; then
    INFO "Clean up old out directory..."
    rm -rf "$OUT_DIR"
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

mkdir -p "$OUT_DIR"
cd "$OUT_DIR"
INFO "output dir: $OUT_DIR"

CMAKE_OPTIONS=(
    -DPICO_SDK_PATH="$PROJECT_ROOT_DIR/libs/pico-sdk"
    -DPICO_BOARD="pico2_w"
    -DPICO_PLATFORM="rp2350"
    -DFREERTOS_KERNEL_PATH="$PROJECT_ROOT_DIR/libs/FreeRTOS-Kernel"
    -DCMAKE_EXPORT_COMPILE_COMMANDS=1
    )

START "Running CMake configuration"
INFO "CMake Options"
for option in "${CMAKE_OPTIONS[@]}"; do
    INFO "$option"
done
cmake "${CMAKE_OPTIONS[@]}" ..
if [ $? -eq 0 ]; then
    ln -sf "$OUT_DIR/compile_commands.json" "$PROJECT_ROOT_DIR/compile_commands.json" 
    DONE "CMake configuration complete"
else
    ERROR "CMake configuration failed"
    exit 1
fi

START "Compiling project"
make -j$(nproc)
if [ $? -eq 0 ]; then
    DONE "Build completed successfully"
else
    ERROR "Build failed"
    exit 1
fi
