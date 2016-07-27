# Copyright 2015-2016 Samsung Electronics Co., Ltd.
# Copyright 2016 University of Szeged.
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

# Remove rdynamic option
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS )

# Components
set(JERRY_CORE    ON  CACHE BOOL "Use jerry-core?")
set(JERRY_LIBC    ON  CACHE BOOL "Use jerry-libc?")
set(JERRY_LIBM    ON  CACHE BOOL "Use jerry-libm?")
set(JERRY_CMDLINE ON  CACHE BOOL "Use jerry command line tool?")
set(UNITTESTS     OFF CACHE BOOL "Use unit tests?")

# Optional build settings
set(PORT_DIR                  "${CMAKE_SOURCE_DIR}/targets/default" CACHE STRING "Should we use default or external port?")
set(COMPILER_DEFAULT_LIBC     OFF                                   CACHE BOOL   "Enable compiler default libc?")
set(ENABLE_LTO                OFF                                   CACHE BOOL   "Enable LTO build?")
set(ENABLE_ALL_IN_ONE         ON                                    CACHE BOOL   "Enable all-in-one build?")
set(ENABLE_STRIP              ON                                    CACHE BOOL   "Discards all symbols from object files?")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

if("${PLATFORM}" STREQUAL "DARWIN")
  set(ENABLE_LTO "OFF")
  set(ENABLE_ALL_IN_ONE "ON")
endif()

# Status messages
message(STATUS "CMAKE_SYSTEM_NAME         " ${CMAKE_SYSTEM_NAME})
message(STATUS "CMAKE_SYSTEM_PROCESSOR    " ${CMAKE_SYSTEM_PROCESSOR})
message(STATUS "CMAKE_BUILD_TYPE          " ${CMAKE_BUILD_TYPE})
message(STATUS "JERRY_CORE                " ${JERRY_CORE})
message(STATUS "JERRY_LIBC                " ${JERRY_LIBC})
message(STATUS "JERRY_LIBM                " ${JERRY_LIBM})
message(STATUS "JERRY_CMDLINE             " ${JERRY_CMDLINE})
message(STATUS "UNITTESTS                 " ${UNITTESTS})
message(STATUS "PORT_DIR                  " ${PORT_DIR})
message(STATUS "COMPILER_DEFAULT_LIBC     " ${COMPILER_DEFAULT_LIBC})
message(STATUS "ENABLE_LTO                " ${ENABLE_LTO})
message(STATUS "ENABLE_ALL_IN_ONE         " ${ENABLE_ALL_IN_ONE})
message(STATUS "ENABLE_STRIP              " ${ENABLE_STRIP})

# Setup directories
# Project binary dir
set(PROJECT_BINARY_DIR "${CMAKE_BINARY_DIR}")

# Library output directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib/")

# Executable output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/")

# Archive targets output Directory
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib/")

# Compile/link flags
# build mode specific compile/link flags
set(CMAKE_C_FLAGS_RELEASE "-Os")

# Architecture-specific compile/link flags
foreach(FLAG ${FLAGS_COMMON_ARCH})
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FLAG}")
endforeach()

# Use jerry libc
if(JERRY_LIBC AND COMPILER_DEFAULT_LIBC)
  message(FATAL_ERROR "JERRY_LIBC and COMPILER_DEFAULT_LIBC is enabled at the same time!")
endif()

# LTO
if(ENABLE_LTO)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
  set(LINKER_FLAGS_COMMON "${LINKER_FLAGS_COMMON} -flto")
  if(CMAKE_COMPILER_IS_GNUCC)
    if(NOT "${PLATFORM}" STREQUAL "DARWIN")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-fat-lto-objects")
    endif()
    # Use gcc-ar and gcc-ranlib to support LTO
    set(CMAKE_AR "gcc-ar")
    set(CMAKE_RANLIB "gcc-ranlib")
  endif()
endif()

# Define _BSD_SOURCE if we use default port and compiler default libc
if(${PORT_DIR} STREQUAL "${CMAKE_SOURCE_DIR}/targets/default" AND COMPILER_DEFAULT_LIBC)
  set(DEFINES_JERRY ${DEFINES_JERRY} _BSD_SOURCE)
endif()

# Imported targets prefix
set(PREFIX_IMPORTED_LIB imported_)

# Imported libraries
if(("${PLATFORM}" STREQUAL "DARWIN") AND (NOT CMAKE_COMPILER_IS_GNUCC))
  # libclang_rt.osx
  set(IMPORTED_LIB "${PREFIX_IMPORTED_LIB}libclang_rt.osx")
  add_library(${IMPORTED_LIB} STATIC IMPORTED)
  execute_process(COMMAND ${CMAKE_C_COMPILER} ${FLAGS_COMMON_ARCH} -print-file-name=
                  OUTPUT_VARIABLE IMPORTED_LIBCLANG_RT_LOCATION
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(IMPORTED_LIBCLANG_RT_LOCATION "${IMPORTED_LIBCLANG_RT_LOCATION}/lib/darwin/libclang_rt.osx.a")
  set_property(TARGET ${IMPORTED_LIB}
               PROPERTY IMPORTED_LOCATION ${IMPORTED_LIBCLANG_RT_LOCATION})
else()
  # libgcc
  set(IMPORTED_LIB "${PREFIX_IMPORTED_LIB}libgcc")
  add_library(${IMPORTED_LIB} STATIC IMPORTED)
  execute_process(COMMAND ${CMAKE_C_COMPILER} ${FLAGS_COMMON_ARCH} -print-file-name=libgcc.a
                  OUTPUT_VARIABLE IMPORTED_LIBGCC_LOCATION
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  set_property(TARGET ${IMPORTED_LIB}
               PROPERTY IMPORTED_LOCATION ${IMPORTED_LIBGCC_LOCATION})
endif()

# Compiler / Linker flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-builtin")
if(("${PLATFORM}" STREQUAL "DARWIN"))
  set(LINKER_FLAGS_COMMON "-lSystem")
else()
  set(LINKER_FLAGS_COMMON "-Wl,-z,noexecstack")
endif()

# Turn off linking to compiler's default libc, in case jerry-libc or external is used
if(NOT COMPILER_DEFAULT_LIBC)
  set(LINKER_FLAGS_COMMON "${LINKER_FLAGS_COMMON} -nostdlib")
endif()

# Turn off stack protector
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-stack-protector")

# Debug information
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -gdwarf-4")

macro(add_jerry_compile_flags)
  foreach(_flag ${ARGV})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${_flag}")
  endforeach()
endmacro()

macro(add_jerry_compile_warnings)
  foreach(_warning ${ARGV})
    add_jerry_compile_flags(-W${_warning})
    if(CMAKE_COMPILER_IS_GNUCC)
      add_jerry_compile_flags(-Werror=${_warning})
    endif()
  endforeach()
endmacro()

add_jerry_compile_warnings(all extra format-nonliteral init-self conversion sign-conversion format-security missing-declarations)
add_jerry_compile_flags(-Wno-stack-protector -Wno-attributes)

if(CMAKE_COMPILER_IS_GNUCC)
  if(JERRY_LIBC)
    add_jerry_compile_flags(-Werror)
  endif()
  add_jerry_compile_warnings(logical-op)
else()
  add_jerry_compile_flags(-Wno-nested-anon-types)
endif()

if(DEFINED EXTERNAL_COMPILE_FLAGS)
  foreach(FLAG ${EXTERNAL_COMPILE_FLAGS})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG}")
  endforeach()
endif()

if(DEFINED EXTERNAL_LINKER_FLAGS)
  foreach(FLAG ${EXTERNAL_LINKER_FLAGS})
    set(LINKER_FLAGS_COMMON "${LINKER_FLAGS_COMMON} ${FLAG}")
  endforeach()
endif()

# C
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -pedantic")

# Strip binary
if(ENABLE_STRIP AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(LINKER_FLAGS_COMMON "${LINKER_FLAGS_COMMON} -s")
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
if(JERRY_CORE)
  add_subdirectory(jerry-core)
endif()

# Jerry commandline tool
if(JERRY_CMDLINE)
  add_subdirectory(jerry-main)
endif()

# Unittests
if(UNITTESTS)
  add_subdirectory(tests/unit)
endif()
