#*****************************************************************************
#
# DESCRIPTION: Script for build tool cmake on both unix and windows
#
#*****************************************************************************
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of either the GNU Lesser General Public License Version 3
# or the Perl Artistic License Version 2.0.
# SPDX-FileCopyrightText: 2003-2025 Wilson Snyder
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
#
#****************************************************************************/

cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW) # Use MSVC_RUNTIME_LIBRARY to select the runtime
project(
    Verilator
    VERSION 5.053
    HOMEPAGE_URL https://verilator.org
    LANGUAGES CXX
)

# This fork builds only the lint half of Verilator, with the MSVC toolchain, for
# Windows x64. Anything else is unsupported, so fail early and clearly rather
# than producing a binary that was never tested.
if(NOT WIN32)
    message(FATAL_ERROR "This lint-only Verilator builds on Windows only.")
endif()
if(NOT MSVC)
    message(FATAL_ERROR "This lint-only Verilator requires MSVC (cl.exe).")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "This lint-only Verilator requires a 64-bit target.")
endif()

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

find_package(Python3 COMPONENTS Interpreter)
set(PYTHON3 ${Python3_EXECUTABLE})
# See also CMake built-in; CMAKE_INSTALL_PREFIX is applied by the install command.
set(CMAKE_INSTALL_DATADIR .)
include(GNUInstallDirs)
include(CheckStructHasMember)
include(FindThreads)

if(DEFINED ENV{WIN_FLEX_BISON})
    set(WIN_FLEX_BISON "$ENV{WIN_FLEX_BISON}")
endif()
if(IS_DIRECTORY ${WIN_FLEX_BISON})
    list(APPEND CMAKE_PREFIX_PATH ${WIN_FLEX_BISON})
else()
    message(
        FATAL_ERROR
        "The path specified by WIN_FLEX_BISON environment variable is not an existing folder: ${WIN_FLEX_BISON}"
    )
endif()
if(NOT WIN_FLEX_BISON)
    message(
        FATAL_ERROR
        "Please install https://github.com/lexxmark/winflexbison and set WIN_FLEX_BISON environment variable."
    )
endif()
set(CMAKE_CXX_STANDARD 20)
# MSVC reports __cplusplus as 199711L unless /Zc:__cplusplus is given, even when
# compiling with /std:c++20. Without it the C++11+ guards in
# include/verilatedos.h silently select their pre-C++11 fallbacks, so for example
# VL_DANGLING() compiles to nothing and leaves a pointer dangling after
# VL_DO_DANGLING(); see V3LinkDot's AstWithParse visitor.
add_compile_options(/Zc:__cplusplus)

set(OBJCACHE "" CACHE STRING "Path for ccache, auto-detected if empty")
option(OBJCACHE_ENABLED "Compile Verilator with ccache" ON)

if(OBJCACHE_ENABLED)
    if(OBJCACHE STREQUAL "")
        find_program(OBJCACHE_PATH ccache)
        if(OBJCACHE_PATH STREQUAL "OBJCACHE_PATH-NOTFOUND")
            set(OBJCACHE_PATH "")
        endif()
    else()
        set(OBJCACHE_PATH "${OBJCACHE}")
    endif()
    if(NOT OBJCACHE_PATH STREQUAL "")
        execute_process(
            COMMAND "${OBJCACHE_PATH}" --version
            OUTPUT_VARIABLE objcache_version
        )
        string(REGEX MATCH "[^\n\r]+" objcache_version "${objcache_version}")
        message(
            STATUS
            "Found ccache: ${OBJCACHE_PATH} (\"${objcache_version}\")"
        )
        set(CMAKE_CXX_COMPILER_LAUNCHER "${OBJCACHE_PATH}")
    endif()
endif()

find_package(BISON)
# Prefer the header installed with the selected generator, including through
# symlinks such as Homebrew's bin/flex. Explicit cache entries take precedence.
find_program(
    FLEX_EXECUTABLE
    NAMES flex win-flex win_flex
    DOC "Path to the flex executable"
)
if(NOT FLEX_EXECUTABLE)
    message(
        FATAL_ERROR
        "Cannot find Flex; set FLEX_EXECUTABLE to its executable path"
    )
endif()
get_filename_component(flex_real_executable "${FLEX_EXECUTABLE}" REALPATH)
get_filename_component(flex_bin_dir "${flex_real_executable}" DIRECTORY)
find_path(
    FLEX_INCLUDE_DIR
    FlexLexer.h
    PATHS "${flex_bin_dir}/../include" "${flex_bin_dir}"
    NO_DEFAULT_PATH
    DOC "Path to the flex headers"
)
find_path(FLEX_INCLUDE_DIR FlexLexer.h)
if(NOT FLEX_INCLUDE_DIR)
    message(
        FATAL_ERROR
        "Cannot find FlexLexer.h; set FLEX_INCLUDE_DIR to its directory"
    )
endif()
mark_as_advanced(FLEX_EXECUTABLE FLEX_INCLUDE_DIR)

# Build

add_subdirectory(src)

# Configuration and Installation

configure_file(include/verilated_config.h.in include/verilated_config.h @ONLY)
configure_file(include/verilated.mk.in include/verilated.mk @ONLY)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/include/verilated_config.h
    DESTINATION include
)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/include/verilated.mk
    DESTINATION include
)

install(PROGRAMS bin/verilator bin/verilator_includer TYPE BIN)

# Development helpers. The GNU make targets that used to provide these went away
# with the autoconf build.
find_program(CLANG_FORMAT_EXECUTABLE clang-format)
file(
    GLOB_RECURSE VERILATOR_FORMAT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)
if(CLANG_FORMAT_EXECUTABLE)
    add_custom_target(
        format
        COMMAND ${CLANG_FORMAT_EXECUTABLE} -i ${VERILATOR_FORMAT_SOURCES}
        COMMENT "Formatting C++ sources with clang-format"
        VERBATIM
    )
    add_custom_target(
        format-check
        COMMAND ${CLANG_FORMAT_EXECUTABLE} --dry-run --Werror
                ${VERILATOR_FORMAT_SOURCES}
        COMMENT "Checking C++ formatting with clang-format"
        VERBATIM
    )
else()
    message(STATUS "clang-format not found; the format targets are unavailable")
endif()

install(
    DIRECTORY include
    TYPE DATA
    FILES_MATCHING
    PATTERN "include/verilated_config.h"
    PATTERN "include/*.[chv]"
    PATTERN "include/*.cpp"
    PATTERN "include/*.vlt"
    PATTERN "include/*.sv"
    PATTERN "include/fstcpp/*.[chv]*"
    PATTERN "include/vltstd/*.[chv]*"
)
