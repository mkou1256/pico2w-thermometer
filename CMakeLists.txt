cmake_minimum_required(VERSION 3.13)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(SRC_DIR ${PROJECT_ROOT}/src)
set(CMAKE_PATH ${PROJECT_ROOT}/cmake)

# ---- SDK Import ----
include(${CMAKE_PATH}/pico_sdk_import.cmake)
include(${CMAKE_PATH}/FreeRTOS_Kernel_import.cmake)
project(pico2w-thermometer C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# ---- Stack size configuration ----
set(PICO_STACK_SIZE 8192)

# ---- Initialize SDK ----
pico_sdk_init()

# ---- Add executable ----
file(GLOB_RECURSE SRC CONFIGURE_DEPENDS ${SRC_DIR}/*.c ${SRC_DIR}/*.cpp)
add_executable(pico2w-thermometer ${SRC})

# ---- Add include directories ----
target_include_directories(pico2w-thermometer PRIVATE ${SRC_DIR})
target_include_directories(pico2w-thermometer PRIVATE ${SRC_DIR}/app)
target_include_directories(pico2w-thermometer PRIVATE ${SRC_DIR}/drivers)
target_include_directories(pico2w-thermometer PRIVATE ${SRC_DIR}/config)
target_include_directories(pico2w-thermometer PRIVATE ${SRC_DIR}/system)
target_include_directories(pico2w-thermometer PRIVATE ${PROJECT_ROOT}/tests)

# ---- Add compiler definitions ----
target_compile_definitions(pico2w-thermometer PRIVATE $<$<CONFIG:Debug>:APP_DEBUG>)

# ---- Add compiler options ----
target_compile_options(pico2w-thermometer PRIVATE -Wall -Wextra)

# ---- Link required libraries ----
target_link_libraries(pico2w-thermometer
    pico_stdlib
    pico_cyw43_arch_none
    hardware_adc
    FreeRTOS-Kernel-Heap4
)

# ---- Enable USB or UART (optional) ----
pico_enable_stdio_usb(pico2w-thermometer 1)
pico_enable_stdio_uart(pico2w-thermometer 0)

# ---- Create UF2 / binary output ----
pico_add_extra_outputs(pico2w-thermometer)
