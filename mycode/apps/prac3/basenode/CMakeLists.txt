# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(basenode)

set(mylib_src_dir ${CMAKE_SOURCE_DIR}/../../mylib)
set(mylib_inc_dir ${CMAKE_SOURCE_DIR}/../../include)

FILE(GLOB app_sources src/*.c)
FILE(GLOB lib_sources
    ${mylib_src_dir}/kalman_filter.c
)

target_sources(app PRIVATE ${app_sources} ${lib_sources})
target_include_directories(app PRIVATE ${mylib_inc_dir})
target_link_libraries(app PRIVATE m)
