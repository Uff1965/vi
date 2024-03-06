cmake_minimum_required(VERSION 3.21 FATAL_ERROR)

project(vi)

cmake_path(SET VI_ROOT_DIR ${PROJECT_SOURCE_DIR})

add_subdirectory(vi_timing)
