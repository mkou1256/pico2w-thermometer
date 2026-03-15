cmake_minimum_required(VERSION 3.13)

set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(SRC_DIR ${PROJECT_ROOT}/src)

# ---- SDK Import ----
include(pico_sdk_import.cmake)
include(FreeRTOS_Kernel_import.cmake)
project(pico2w-thermometer C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# ---- Stack size configuration ----
set(PICO_STACK_SIZE 8192)

# ---- Initialize SDK ----
pico_sdk_init()

# ---- Add executable ----
file(GLOB SRC CONFIGURE_DEPENDS ${SRC_DIR}/*.c ${SRC_DIR}/*.cpp)
file(GLOB APP CONFIGURE_DEPENDS ${SRC_DIR}/app/*.c ${SRC_DIR}/app/*.cpp)
file(GLOB KERNEL CONFIGURE_DEPENDS ${SRC_DIR}/kernel/*.c ${SRC_DIR}/kernel/*.cpp)
file(GLOB DRI CONFIGURE_DEPENDS ${SRC_DIR}/driver/*.c ${SRC_DIR}/driver/*.cpp)

add_executable(pico2w-thermometer 
    ${SRC}
    ${APP}
    ${KERNEL}
    ${DRI}
)

# ---- Add include directories ----
target_include_directories(pico2w-thermometer PRIVATE ${SRC_DIR})
target_include_directories(pico2w-thermometer PRIVATE ${SRC_DIR}/app)
target_include_directories(pico2w-thermometer PRIVATE ${SRC_DIR}/kernel)
target_include_directories(pico2w-thermometer PRIVATE ${SRC_DIR}/driver)
target_include_directories(pico2w-thermometer PRIVATE ${PROJECT_ROOT}/config)
target_include_directories(pico2w-thermometer PRIVATE ${PROJECT_ROOT}/tests)

# ---- Link required libraries ----
target_link_libraries(pico2w-thermometer
    pico_stdlib
    pico_cyw43_arch_none
    FreeRTOS-Kernel-Heap4
)

# ---- Enable USB or UART (optional) ----
pico_enable_stdio_usb(pico2w-thermometer 1)
pico_enable_stdio_uart(pico2w-thermometer 0)

# ---- Create UF2 / binary output ----
pico_add_extra_outputs(pico2w-thermometer)
