# Copyright JS Foundation and other contributors, http://js.foundation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required (VERSION 2.8.12)
project (Jerry C ASM)

# Determining platform
set(PLATFORM "${CMAKE_SYSTEM_NAME}")
string(TOUPPER "${PLATFORM}" PLATFORM)

# Determining compiler
if(CMAKE_C_COMPILER_ID MATCHES "GNU")
  set(USING_GCC 1)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
  set(USING_CLANG 1)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "TI")
  set(USING_TI 1)
endif()

# Determining build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "MinSizeRel")
endif()

# Optional components
set(JERRY_CMDLINE           ON  CACHE BOOL "Build jerry command line tool?")
set(JERRY_CMDLINE_TEST      OFF CACHE BOOL "Build jerry test command line tool?")
set(JERRY_CMDLINE_SNAPSHOT  OFF CACHE BOOL "Build jerry snapshot command line tool?")
set(JERRY_PORT_DEFAULT      ON  CACHE BOOL "Build default jerry port implementation?")
set(JERRY_EXT               ON  CACHE BOOL "Build jerry-ext?")
set(JERRY_LIBC              ON  CACHE BOOL "Build and use jerry-libc?")
set(JERRY_LIBM              ON  CACHE BOOL "Build and use jerry-libm?")
set(UNITTESTS               OFF CACHE BOOL "Build unit tests?")
set(DOCTESTS                OFF CACHE BOOL "Build doc tests?")

# Optional build settings
set(ENABLE_ALL_IN_ONE  OFF CACHE BOOL "Enable all-in-one build?")
set(ENABLE_LTO         ON  CACHE BOOL "Enable LTO build?")
set(ENABLE_STATIC_LINK ON  CACHE BOOL "Enable static linking?")
set(ENABLE_STRIP       ON  CACHE BOOL "Enable stripping all symbols from release binary?")

# Optional features
set(FEATURE_INIT_FINI OFF CACHE BOOL "Enable init/fini arrays?")

# Option overrides
if(JERRY_CMDLINE OR JERRY_CMDLINE_TEST OR JERRY_CMDLINE_SNAPSHOT OR UNITTESTS OR DOCTESTS)
  set(JERRY_PORT_DEFAULT ON)

  set(JERRY_PORT_DEFAULT_MESSAGE " (FORCED BY CMDLINE OR TESTS)")
endif()

if(JERRY_CMDLINE OR DOCTESTS)
  set(JERRY_EXT ON)

  set(JERRY_EXT_MESSAGE " (FORCED BY CMDLINE OR TESTS)")
endif()

if("${PLATFORM}" STREQUAL "DARWIN")
  set(JERRY_LIBC         OFF)
  set(JERRY_LIBM         OFF)
  set(ENABLE_ALL_IN_ONE  ON)
  set(ENABLE_LTO         OFF)
  set(ENABLE_STATIC_LINK OFF)
  set(ENABLE_STRIP       OFF)

  set(JERRY_LIBC_MESSAGE         " (FORCED BY PLATFORM)")
  set(JERRY_LIBM_MESSAGE         " (FORCED BY PLATFORM)")
  set(ENABLE_ALL_IN_ONE_MESSAGE  " (FORCED BY PLATFORM)")
  set(ENABLE_LTO_MESSAGE         " (FORCED BY PLATFORM)")
  set(ENABLE_STATIC_LINK_MESSAGE " (FORCED BY PLATFORM)")
  set(ENABLE_STRIP_MESSAGE       " (FORCED BY PLATFORM)")
endif()

if(USING_TI)
  set(ENABLE_STATIC_LINK ON)
  set(ENABLE_STRIP       OFF)

  set(ENABLE_STATIC_LINK_MESSAGE " (FORCED BY COMPILER)")
  set(ENABLE_STRIP_MESSAGE       " (FORCED BY COMPILER)")
endif()

# Status messages
message(STATUS "CMAKE_BUILD_TYPE          " ${CMAKE_BUILD_TYPE})
message(STATUS "CMAKE_SYSTEM_NAME         " ${CMAKE_SYSTEM_NAME})
message(STATUS "CMAKE_SYSTEM_PROCESSOR    " ${CMAKE_SYSTEM_PROCESSOR})
message(STATUS "ENABLE_ALL_IN_ONE         " ${ENABLE_ALL_IN_ONE} ${ENABLE_ALL_IN_ONE_MESSAGE})
message(STATUS "ENABLE_LTO                " ${ENABLE_LTO} ${ENABLE_LTO_MESSAGE})
message(STATUS "ENABLE_STATIC_LINK        " ${ENABLE_STATIC_LINK} ${ENABLE_STATIC_LINK_MESSAGE})
message(STATUS "ENABLE_STRIP              " ${ENABLE_STRIP} ${ENABLE_STRIP_MESSAGE})
message(STATUS "JERRY_CMDLINE             " ${JERRY_CMDLINE})
message(STATUS "JERRY_CMDLINE_TEST        " ${JERRY_CMDLINE_TEST})
message(STATUS "JERRY_CMDLINE_SNAPSHOT    " ${JERRY_CMDLINE_SNAPSHOT})
message(STATUS "JERRY_PORT_DEFAULT        " ${JERRY_PORT_DEFAULT} ${JERRY_PORT_DEFAULT_MESSAGE})
message(STATUS "JERRY_EXT                 " ${JERRY_EXT} ${JERRY_EXT_MESSAGE})
message(STATUS "JERRY_LIBC                " ${JERRY_LIBC} ${JERRY_LIBC_MESSAGE})
message(STATUS "JERRY_LIBM                " ${JERRY_LIBM} ${JERRY_LIBM_MESSAGE})
message(STATUS "UNITTESTS                 " ${UNITTESTS})
message(STATUS "DOCTESTS                  " ${DOCTESTS})
message(STATUS "FEATURE_INIT_FINI         " ${FEATURE_INIT_FINI})

# Setup directories
# Project binary dir
set(PROJECT_BINARY_DIR "${CMAKE_BINARY_DIR}")

# Library output directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib/")

# Executable output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/")

# Archive targets output Directory
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib/")

# Remove rdynamic option
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS )

# Compile/link flags
# Helper macros
macro(jerry_add_flags VAR)
  foreach(_flag ${ARGN})
    set(${VAR} "${${VAR}} ${_flag}")
  endforeach()
endmacro()

macro(jerry_add_compile_flags)
  jerry_add_flags(CMAKE_C_FLAGS ${ARGV})
endmacro()

macro(jerry_add_compile_warnings)
  foreach(_warning ${ARGV})
    jerry_add_compile_flags(-W${_warning})
    if(USING_GCC)
      jerry_add_compile_flags(-Werror=${_warning})
    endif()
  endforeach()
endmacro()

macro(jerry_add_link_flags)
  jerry_add_flags(LINKER_FLAGS_COMMON ${ARGV})
endmacro()

# Architecture-specific compile/link flags
jerry_add_compile_flags(${FLAGS_COMMON_ARCH})
jerry_add_flags(CMAKE_EXE_LINKER_FLAGS ${FLAGS_COMMON_ARCH})

# Enable static build
if(ENABLE_STATIC_LINK)
  if (USING_GCC OR USING_CLANG)
    jerry_add_link_flags("-static")
  endif()
endif()

# LTO
if(ENABLE_LTO)
  if (USING_GCC OR USING_CLANG)
    jerry_add_compile_flags(-flto)
    jerry_add_link_flags(-flto)
  endif()
  if(USING_GCC)
    jerry_add_compile_flags(-fno-fat-lto-objects)
    # Use gcc-ar and gcc-ranlib to support LTO
    set(CMAKE_AR "gcc-ar")
    set(CMAKE_RANLIB "gcc-ranlib")
  endif()
  if(USING_TI)
    jerry_add_link_flags(-lto)
  endif()
endif()

# Compiler / Linker flags
if (USING_GCC OR USING_CLANG)
  jerry_add_compile_flags(-fno-builtin)
endif()
if(("${PLATFORM}" STREQUAL "DARWIN"))
  jerry_add_link_flags(-lSystem)
  set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> Sqc <TARGET> <LINK_FLAGS> <OBJECTS>")
  set(CMAKE_C_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
else()
  jerry_add_link_flags(-Wl,-z,noexecstack)
endif()

# Turn off linking to compiler's default libc, in case jerry-libc is used
if(JERRY_LIBC)
  jerry_add_link_flags(-nostdlib)
endif()

# Turn off stack protector
if (USING_GCC OR USING_CLANG)
jerry_add_compile_flags(-fno-stack-protector)
endif()

if (USING_GCC OR USING_CLANG)
  jerry_add_compile_warnings(all extra format-nonliteral init-self conversion sign-conversion format-security missing-declarations shadow strict-prototypes undef old-style-definition)
  jerry_add_compile_flags(-Wno-stack-protector -Wno-attributes)
endif()

if(USING_GCC)
  jerry_add_compile_warnings(logical-op)
elseif(USING_CLANG)
  jerry_add_compile_flags(-Wno-nested-anon-types -Wno-static-in-inline)
endif()

if(JERRY_LIBC)
  jerry_add_compile_flags(-Werror)
endif()

# C
if (USING_GCC OR USING_CLANG)
  jerry_add_compile_flags(-std=c99 -pedantic)
elseif(USING_TI)
  jerry_add_compile_flags(--c99)
endif()

# Strip binary
if(ENABLE_STRIP AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
  jerry_add_link_flags(-s)
endif()

# TODO: Remove workaround for gcc 7 bug if the
# fallthrough comment detection is fixed.
if (USING_GCC AND CMAKE_C_COMPILER_VERSION VERSION_GREATER 7.0)
  jerry_add_compile_flags(-Wno-implicit-fallthrough)
endif()

# External compiler & linker flags
if(DEFINED EXTERNAL_COMPILE_FLAGS)
  jerry_add_compile_flags(${EXTERNAL_COMPILE_FLAGS})
endif()

if(DEFINED EXTERNAL_LINKER_FLAGS)
  jerry_add_link_flags(${EXTERNAL_LINKER_FLAGS})
endif()

# Jerry's libc
if(JERRY_LIBC)
  add_subdirectory(jerry-libc)
endif()

# Jerry's libm
if(JERRY_LIBM)
  add_subdirectory(jerry-libm)
endif()

# Jerry's core
add_subdirectory(jerry-core)

# Jerry's default port implementation
if(JERRY_PORT_DEFAULT)
  add_subdirectory(jerry-port/default)
endif()

# Jerry's extension tools
if(JERRY_EXT)
  add_subdirectory(jerry-ext)
endif()

# Jerry command line tool
if(JERRY_CMDLINE OR JERRY_CMDLINE_TEST OR JERRY_CMDLINE_SNAPSHOT)
  add_subdirectory(jerry-main)
endif()

# Unittests
if(UNITTESTS)
  add_subdirectory(tests/unit-core)
  if(JERRY_LIBM)
    add_subdirectory(tests/unit-libm)
  endif()
  if(JERRY_EXT)
    add_subdirectory(tests/unit-ext)
  endif()
endif()

# Doctests
if(DOCTESTS)
  add_subdirectory(tests/unit-doc)
endif()
