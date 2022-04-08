cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)

include(pico_sdk_import.cmake)

project(misc)

pico_sdk_init()

add_definitions(-DPI_PICO)

include_directories(../lib)
link_directories(../lib/build)
add_subdirectory(cnc-touch-probe)
