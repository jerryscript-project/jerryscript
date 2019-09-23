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
project (Jerry C)

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

if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
  set(USING_MSVC 1)
endif()

# Determining build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "MinSizeRel")
endif()

# Optional components
set(JERRY_CMDLINE           ON  CACHE BOOL "Build jerry command line tool?")
set(JERRY_CMDLINE_TEST      OFF CACHE BOOL "Build jerry test command line tool?")
set(JERRY_CMDLINE_SNAPSHOT  OFF CACHE BOOL "Build jerry snapshot command line tool?")
set(JERRY_LIBFUZZER         OFF CACHE BOOL "Build jerry with libfuzzer support?")
set(JERRY_PORT_DEFAULT      ON  CACHE BOOL "Build default jerry port implementation?")
set(JERRY_EXT               ON  CACHE BOOL "Build jerry-ext?")
set(JERRY_LIBM              ON  CACHE BOOL "Build and use jerry-libm?")
set(UNITTESTS               OFF CACHE BOOL "Build unit tests?")
set(DOCTESTS                OFF CACHE BOOL "Build doc tests?")

# Optional build settings
set(BUILD_SHARED_LIBS  OFF CACHE BOOL "Build shared libraries?")
set(ENABLE_LTO         ON  CACHE BOOL "Enable LTO build?")
set(ENABLE_STRIP       ON  CACHE BOOL "Enable stripping all symbols from release binary?")

# Option overrides
if(NOT USING_CLANG)
  set(JERRY_LIBFUZZER OFF)

  set(JERRY_LIBFUZZER_MESSAGE " (FORCED BY COMPILER)")
endif()

if(JERRY_CMDLINE OR JERRY_CMDLINE_TEST OR JERRY_CMDLINE_SNAPSHOT OR JERRY_LIBFUZZER OR UNITTESTS OR DOCTESTS)
  set(JERRY_PORT_DEFAULT ON)

  set(JERRY_PORT_DEFAULT_MESSAGE " (FORCED BY CMDLINE OR LIBFUZZER OR TESTS)")
endif()

if(JERRY_CMDLINE OR DOCTESTS)
  set(JERRY_EXT ON)

  set(JERRY_EXT_MESSAGE " (FORCED BY CMDLINE OR TESTS)")
endif()

if("${PLATFORM}" STREQUAL "DARWIN")
  set(JERRY_LIBM         OFF)
  set(ENABLE_LTO         OFF)
  set(ENABLE_STRIP       OFF)

  set(JERRY_LIBM_MESSAGE         " (FORCED BY PLATFORM)")
  set(ENABLE_LTO_MESSAGE         " (FORCED BY PLATFORM)")
  set(ENABLE_STRIP_MESSAGE       " (FORCED BY PLATFORM)")
endif()

if(USING_TI)
  set(ENABLE_STRIP       OFF)

  set(ENABLE_STRIP_MESSAGE       " (FORCED BY COMPILER)")
endif()

if(USING_MSVC)
  set(JERRY_LIBM         OFF)
  set(ENABLE_STRIP       OFF)

  set(JERRY_LIBM_MESSAGE         " (FORCED BY COMPILER)")
  set(ENABLE_STRIP_MESSAGE       " (FORCED BY COMPILER)")
endif()

if(CYGWIN)
  set(ENABLE_LTO         OFF)

  set(ENABLE_LTO_MESSAGE         " (FORCED BY PLATFORM)")
endif()

# Status messages
message(STATUS "CMAKE_BUILD_TYPE            " ${CMAKE_BUILD_TYPE})
message(STATUS "CMAKE_C_COMPILER_ID         " ${CMAKE_C_COMPILER_ID})
message(STATUS "CMAKE_SYSTEM_NAME           " ${CMAKE_SYSTEM_NAME})
message(STATUS "CMAKE_SYSTEM_PROCESSOR      " ${CMAKE_SYSTEM_PROCESSOR})
message(STATUS "BUILD_SHARED_LIBS           " ${BUILD_SHARED_LIBS})
message(STATUS "ENABLE_LTO                  " ${ENABLE_LTO} ${ENABLE_LTO_MESSAGE})
message(STATUS "ENABLE_STRIP                " ${ENABLE_STRIP} ${ENABLE_STRIP_MESSAGE})
message(STATUS "JERRY_CMDLINE               " ${JERRY_CMDLINE} ${JERRY_CMDLINE_MESSAGE})
message(STATUS "JERRY_CMDLINE_TEST          " ${JERRY_CMDLINE_TEST} ${JERRY_CMDLINE_TEST_MESSAGE})
message(STATUS "JERRY_CMDLINE_SNAPSHOT      " ${JERRY_CMDLINE_SNAPSHOT} ${JERRY_CMDLINE_SNAPSHOT_MESSAGE})
message(STATUS "JERRY_LIBFUZZER             " ${JERRY_LIBFUZZER} ${JERRY_LIBFUZZER_MESSAGE})
message(STATUS "JERRY_PORT_DEFAULT          " ${JERRY_PORT_DEFAULT} ${JERRY_PORT_DEFAULT_MESSAGE})
message(STATUS "JERRY_EXT                   " ${JERRY_EXT} ${JERRY_EXT_MESSAGE})
message(STATUS "JERRY_LIBM                  " ${JERRY_LIBM} ${JERRY_LIBM_MESSAGE})
message(STATUS "UNITTESTS                   " ${UNITTESTS})
message(STATUS "DOCTESTS                    " ${DOCTESTS})

# Setup directories
# Note: This mimics a conventional file system layout in the build directory for
# the sake of convenient location of build artefacts. Proper installation to
# traditional locations is also supported, e.g., to /usr/local.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/")

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
    jerry_add_compile_flags(-Werror=${_warning})
  endforeach()
endmacro()

macro(jerry_add_link_flags)
  jerry_add_flags(LINKER_FLAGS_COMMON ${ARGV})
endmacro()

# Architecture-specific compile/link flags
jerry_add_compile_flags(${FLAGS_COMMON_ARCH})
jerry_add_flags(CMAKE_EXE_LINKER_FLAGS ${FLAGS_COMMON_ARCH})

# LTO
if(ENABLE_LTO)
  if(USING_GCC OR USING_CLANG)
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
if("${PLATFORM}" STREQUAL "DARWIN")
  jerry_add_link_flags(-lSystem)
  set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> Sqc <TARGET> <LINK_FLAGS> <OBJECTS>")
  set(CMAKE_C_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
  set(CMAKE_SHARED_LINKER_FLAGS  "-undefined dynamic_lookup")
elseif(NOT CYGWIN AND (USING_GCC OR USING_CLANG))
  jerry_add_link_flags(-Wl,-z,noexecstack)
endif()

if(USING_GCC OR USING_CLANG)
  jerry_add_compile_flags(-std=c99 -pedantic)
  # Turn off stack protector
  jerry_add_compile_flags(-fno-builtin -fno-stack-protector)
  jerry_add_compile_warnings(all extra format-nonliteral init-self conversion sign-conversion format-security missing-declarations shadow strict-prototypes undef old-style-definition)
  jerry_add_compile_flags(-Wno-stack-protector -Wno-attributes -Werror)
endif()

if(USING_GCC)
  jerry_add_compile_warnings(logical-op)
  # TODO: Remove workaround for gcc 7 bug if the fallthrough comment detection is fixed.
  if(CMAKE_C_COMPILER_VERSION VERSION_GREATER 7.0)
    jerry_add_compile_flags(-Wno-implicit-fallthrough)
  endif()
endif()

if(USING_CLANG)
  jerry_add_compile_flags(-Wno-nested-anon-types -Wno-static-in-inline)
endif()

if(USING_TI)
  jerry_add_compile_flags(--c99)
endif()

if(USING_MSVC)
  jerry_add_link_flags(/OPT:NOREF)
  # Disable MSVC warning 4996 globally because it stops us from using standard C functions.
  jerry_add_compile_flags(/wd4996)
endif()

if(JERRY_LIBFUZZER)
  jerry_add_compile_flags(-fsanitize=fuzzer-no-link)
endif()

# Strip binary
if(ENABLE_STRIP AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
  jerry_add_link_flags(-s)
endif()

# External compiler & linker flags
if(DEFINED EXTERNAL_COMPILE_FLAGS)
  jerry_add_compile_flags(${EXTERNAL_COMPILE_FLAGS})
endif()

if(DEFINED EXTERNAL_LINKER_FLAGS)
  jerry_add_link_flags(${EXTERNAL_LINKER_FLAGS})
endif()

# Used for placeholder to attach single source build targets
add_custom_target(generate-single-source)

# Jerry's libm
if(JERRY_LIBM)
  add_subdirectory(jerry-libm)
endif()

# Jerry's core
add_subdirectory(jerry-core)

# Jerry's extension tools
if(JERRY_EXT)
  add_subdirectory(jerry-ext)
endif()

# Jerry's default port implementation
if(JERRY_PORT_DEFAULT)
  add_subdirectory(jerry-port/default)
endif()

# Jerry command line tool
if(JERRY_CMDLINE OR JERRY_CMDLINE_TEST OR JERRY_CMDLINE_SNAPSHOT OR JERRY_LIBFUZZER)
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
