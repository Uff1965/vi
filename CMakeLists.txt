cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

project(vi)

set(CMAKE_CONFIGURATION_TYPES Debug Release)
set(CMAKE_BUILD_TYPE "Release" CACHE STRING)

get_filename_component(VI_ROOT_DIR "${PROJECT_SOURCE_DIR}/." DIRECTORY)
set(VI_OUT_DIR "${VI_ROOT_DIR}/lib")

add_subdirectory(vi_timing)
