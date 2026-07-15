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

# オプション解析
while [ $# -gt 0 ]; do
  case "$1" in
    --fullbuild|-f)
      FULLBUILD=true
      shift
      ;;
    *)
      echo "Unknown option: $1"
      exit 1
      ;;
  esac
done

if [ "$FULLBUILD" = true ]; then
    INFO "Clean up old build directory..."
    rm -rf "$BUILD_DIR"
fi

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
cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G Ninja "${CMAKE_OPTIONS[@]}"

START "Compiling project"
cmake --build "$BUILD_DIR" -j"$(nproc)"

DONE "Build completed successfully"
