cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(seminar)

# Collect all the source files in app/prac2/src
FILE(GLOB app_sources src/*.c)

# Collect all the source files in lib. Note that
# the path given needs to be relative to the
# CMakeLists root, which is app/prac2 here,
# hence the ../../lib.c.
FILE(GLOB lib_sources ../../lib/*.c)

# Tell CMake to build with the app and lib sources
target_sources(app PRIVATE ${app_sources} ${lib_sources}) 
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
# Tell CMake where our header files are
target_include_directories(app PRIVATE ../../include)