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
set(JERRY_CORE_NAME jerry-core)
project (${JERRY_CORE_NAME} C)

# Optional features
set(FEATURE_PROFILE         "full" CACHE STRING "ES5.1 profile: full, compact, minimal")
set(FEATURE_ERROR_MESSAGES  OFF    CACHE BOOL   "Enable error messages?")
set(FEATURE_LOG             OFF    CACHE BOOL   "Enable logging?")
set(FEATURE_VALGRIND        OFF    CACHE BOOL   "Enable Valgrind support?")
set(FEATURE_VALGRIND_FREYA  OFF    CACHE BOOL   "Enable Valgrind-Freya support?")
set(FEATURE_MEM_STRESS_TEST OFF    CACHE BOOL   "Enable mem-stress test?")
set(FEATURE_MEM_STATS       OFF    CACHE BOOL   "Enable memory-statistics?")
set(FEATURE_SNAPSHOT_SAVE   OFF    CACHE BOOL   "Allow to save snapshot files?")
set(FEATURE_SNAPSHOT_EXEC   OFF    CACHE BOOL   "Allow to execute snapshot files?")
set(MEM_HEAP_SIZE_KB        "512"  CACHE STRING "Size of memory heap, in kilobytes")

# Status messages
message(STATUS "FEATURE_PROFILE           " ${FEATURE_PROFILE})
message(STATUS "FEATURE_ERROR_MESSAGES    " ${FEATURE_ERROR_MESSAGES})
message(STATUS "FEATURE_LOG               " ${FEATURE_LOG})
message(STATUS "FEATURE_VALGRIND          " ${FEATURE_VALGRIND})
message(STATUS "FEATURE_VALGRIND_FREYA    " ${FEATURE_VALGRIND_FREYA})
message(STATUS "FEATURE_MEM_STRESS_TEST   " ${FEATURE_MEM_STRESS_TEST})
message(STATUS "FEATURE_MEM_STATS         " ${FEATURE_MEM_STATS})
message(STATUS "FEATURE_SNAPSHOT_SAVE     " ${FEATURE_SNAPSHOT_SAVE})
message(STATUS "FEATURE_SNAPSHOT_EXEC     " ${FEATURE_SNAPSHOT_EXEC})
message(STATUS "MEM_HEAP_SIZE_KB          " ${MEM_HEAP_SIZE_KB})

# Include directories
set(INCLUDE_CORE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/ecma/base"
    "${CMAKE_CURRENT_SOURCE_DIR}/ecma/builtin-objects"
    "${CMAKE_CURRENT_SOURCE_DIR}/ecma/operations"
    "${CMAKE_CURRENT_SOURCE_DIR}/jcontext"
    "${CMAKE_CURRENT_SOURCE_DIR}/jmem"
    "${CMAKE_CURRENT_SOURCE_DIR}/jrt"
    "${CMAKE_CURRENT_SOURCE_DIR}/lit"
    "${CMAKE_CURRENT_SOURCE_DIR}/parser/js"
    "${CMAKE_CURRENT_SOURCE_DIR}/parser/regexp"
    "${CMAKE_CURRENT_SOURCE_DIR}/vm")

# Sources
# Jerry core
file(GLOB SOURCE_CORE_API                   *.c)
file(GLOB SOURCE_CORE_ECMA_BASE             ecma/base/*.c)
file(GLOB SOURCE_CORE_ECMA_BUILTINS         ecma/builtin-objects/*.c)
file(GLOB SOURCE_CORE_ECMA_OPERATIONS       ecma/operations/*.c)
file(GLOB SOURCE_CORE_JCONTEXT              jcontext/*.c)
file(GLOB SOURCE_CORE_JMEM                  jmem/*.c)
file(GLOB SOURCE_CORE_JRT                   jrt/*.c)
file(GLOB SOURCE_CORE_LIT                   lit/*.c)
file(GLOB SOURCE_CORE_PARSER_JS             parser/js/*.c)
file(GLOB SOURCE_CORE_PARSER_REGEXP         parser/regexp/*.c)
file(GLOB SOURCE_CORE_VM                    vm/*.c)

set(SOURCE_CORE_FILES
    ${SOURCE_CORE_API}
    ${SOURCE_CORE_ECMA_BASE}
    ${SOURCE_CORE_ECMA_BUILTINS}
    ${SOURCE_CORE_ECMA_OPERATIONS}
    ${SOURCE_CORE_JCONTEXT}
    ${SOURCE_CORE_JMEM}
    ${SOURCE_CORE_JRT}
    ${SOURCE_CORE_LIT}
    ${SOURCE_CORE_PARSER_JS}
    ${SOURCE_CORE_PARSER_REGEXP}
    ${SOURCE_CORE_VM})

# Jerry port
file(GLOB SOURCE_PORT_FILES "${PORT_DIR}/*.c")

# All-in-one build
if(ENABLE_ALL_IN_ONE)
  set(ALL_IN_FILE "${CMAKE_BINARY_DIR}/jerry-all-in.c")
  list(SORT SOURCE_CORE_FILES)
  file(REMOVE ${ALL_IN_FILE})

  foreach(FILE ${SOURCE_CORE_FILES})
    file(APPEND ${ALL_IN_FILE} "#include \"${FILE}\"\n")
  endforeach()

  foreach(FILE ${SOURCE_PORT_FILES})
    file(APPEND ${ALL_IN_FILE} "#include \"${FILE}\"\n")
  endforeach()

  set(SOURCE_CORE ${ALL_IN_FILE})
else()
  set(SOURCE_CORE ${SOURCE_CORE_FILES} ${SOURCE_PORT_FILES})
endif()

# Third-party
# Valgrind
set(INCLUDE_THIRD_PARTY_VALGRIND "${CMAKE_SOURCE_DIR}/third-party/valgrind")

# Definitions
# Get version information from git
if(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/.git")
  execute_process(COMMAND git symbolic-ref -q HEAD
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                  OUTPUT_VARIABLE JERRY_GIT_BRANCH
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND git rev-parse HEAD
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                  OUTPUT_VARIABLE JERRY_GIT_COMMIT
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
  set(JERRY_GIT_BRANCH "UNDEFINED")
  set(JERRY_GIT_COMMIT "UNDEFINED")
endif()

# Get build date
execute_process(COMMAND date +%d/%m/%Y
                OUTPUT_VARIABLE JERRY_BUILD_DATE
                OUTPUT_STRIP_TRAILING_WHITESPACE)

set(DEFINES_JERRY
    ${DEFINES_JERRY}
    JERRY_BUILD_DATE="${JERRY_BUILD_DATE}"
    JERRY_COMMIT_HASH="${JERRY_GIT_COMMIT}"
    JERRY_BRANCH_NAME="${JERRY_GIT_BRANCH}")

# build mode specific compile/link flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_ENABLE_PRETTY_PRINTER)
else()
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_NDEBUG)
endif()

# ES5.1 profiles
# Full profile
if(FEATURE_PROFILE STREQUAL "full")
  set(DEFINES_JERRY ${DEFINES_JERRY} CONFIG_ECMA_NUMBER_TYPE=CONFIG_ECMA_NUMBER_FLOAT64)

# Compact profile
elseif(FEATURE_PROFILE STREQUAL "compact")
  set(DEFINES_JERRY ${DEFINES_JERRY} CONFIG_ECMA_COMPACT_PROFILE)

# Minimal compact profile
elseif(FEATURE_PROFILE STREQUAL "minimal")
  set(DEFINES_JERRY
      ${DEFINES_JERRY}
      CONFIG_ECMA_COMPACT_PROFILE
#
# Date and RegExp built-in objects are also disabled in non-minimal compact profile build
#
# CONFIG_ECMA_COMPACT_PROFILE_DISABLE_DATE_BUILTIN
# CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN
#
      CONFIG_ECMA_COMPACT_PROFILE_DISABLE_NUMBER_BUILTIN
      CONFIG_ECMA_COMPACT_PROFILE_DISABLE_STRING_BUILTIN
      CONFIG_ECMA_COMPACT_PROFILE_DISABLE_BOOLEAN_BUILTIN
      CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ERROR_BUILTINS
      CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ARRAY_BUILTIN
      CONFIG_ECMA_COMPACT_PROFILE_DISABLE_MATH_BUILTIN
      CONFIG_ECMA_COMPACT_PROFILE_DISABLE_JSON_BUILTIN)

else()
  message(FATAL_ERROR "FEATURE_PROFILE='${FEATURE_PROFILE}' doesn't supported")
endif()

# Jerry heap-section
if(DEFINED JERRY_HEAP_SECTION_ATTR)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_HEAP_SECTION_ATTR=${JERRY_HEAP_SECTION_ATTR})
endif()

# Fill error messages for builtin error objects
if(FEATURE_ERROR_MESSAGES)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_ENABLE_ERROR_MESSAGES)
endif()

# Log
if(FEATURE_LOG)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_ENABLE_LOG)
endif()

# Valgrind
if(FEATURE_VALGRIND)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_VALGRIND)
  set(INCLUDE_CORE ${INCLUDE_CORE} ${INCLUDE_THIRD_PARTY_VALGRIND})
endif()

# Valgrind Freya
if(FEATURE_VALGRIND_FREYA)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_VALGRIND_FREYA)
  set(INCLUDE_CORE ${INCLUDE_CORE} ${INCLUDE_THIRD_PARTY_VALGRIND})
endif()

# Memory management stress-test mode
if(FEATURE_MEM_STRESS_TEST)
  set(DEFINES_JERRY ${DEFINES_JERRY} JMEM_GC_BEFORE_EACH_ALLOC)
endif()

# Memory statistics
if(FEATURE_MEM_STATS)
  set(DEFINES_JERRY ${DEFINES_JERRY} JMEM_STATS)
endif()

# Snapshot save
if(FEATURE_SNAPSHOT_SAVE)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_ENABLE_SNAPSHOT_SAVE)
endif()

# Snapshot exec
if(FEATURE_SNAPSHOT_EXEC)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_ENABLE_SNAPSHOT_EXEC)
endif()

# Size of heap
math(EXPR MEM_HEAP_AREA_SIZE "${MEM_HEAP_SIZE_KB} * 1024")
set(DEFINES_JERRY ${DEFINES_JERRY} CONFIG_MEM_HEAP_AREA_SIZE=${MEM_HEAP_AREA_SIZE})

add_library(${JERRY_CORE_NAME} STATIC ${SOURCE_CORE})

target_compile_definitions(${JERRY_CORE_NAME} PUBLIC ${DEFINES_JERRY})
target_include_directories(${JERRY_CORE_NAME} PUBLIC ${INCLUDE_CORE})

if (JERRY_LIBC)
  target_include_directories(${JERRY_CORE_NAME} SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/jerry-libc/include")
endif()

