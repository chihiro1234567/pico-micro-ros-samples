# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(micro-ros-sample2 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# micro-ros library
link_directories(libmicroros)

# Add executable. Default name is the project name, version 0.1

add_executable(micro-ros-sample2
    micro-ros-sample2.c
    pico_uart_transport.c
)

pico_set_program_name(micro-ros-sample2 "micro-ros-sample2")
pico_set_program_version(micro-ros-sample2 "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(micro-ros-sample2 1)
pico_enable_stdio_usb(micro-ros-sample2 0)

# Add the standard library to the build
target_link_libraries(micro-ros-sample2
        pico_stdlib
        microros
)

# Add the standard include files to the build
target_include_directories(micro-ros-sample2 PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        libmicroros/include
)

SET(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
add_compile_definitions(PICO_UART_ENABLE_CRLF_SUPPORT=0)
add_compile_definitions(PICO_STDIO_ENABLE_CRLF_SUPPORT=0)
add_compile_definitions(PICO_STDIO_DEFAULT_CRLF=0)

pico_add_extra_outputs(micro-ros-sample2)

