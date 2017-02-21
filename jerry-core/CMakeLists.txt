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
set(JERRY_CORE_NAME jerry-core)
project (${JERRY_CORE_NAME} C)

# Optional features
set(FEATURE_CPOINTER_32_BIT  OFF     CACHE BOOL   "Enable 32 bit compressed pointers?")
set(FEATURE_DEBUGGER         OFF     CACHE BOOL   "Enable JerryScript debugger?")
set(FEATURE_DEBUGGER_PORT    "5001"  CACHE STRING "Set debugger port number (default: 5001)")
set(FEATURE_ERROR_MESSAGES   OFF     CACHE BOOL   "Enable error messages?")
set(FEATURE_JS_PARSER        ON      CACHE BOOL   "Enable js-parser?")
set(FEATURE_MEM_STATS        OFF     CACHE BOOL   "Enable memory statistics?")
set(FEATURE_MEM_STRESS_TEST  OFF     CACHE BOOL   "Enable mem-stress test?")
set(FEATURE_PARSER_DUMP      OFF     CACHE BOOL   "Enable parser byte-code dumps?")
set(FEATURE_PROFILE          "es5.1" CACHE STRING "Use default or other profile?")
set(FEATURE_REGEXP_DUMP      OFF     CACHE BOOL   "Enable regexp byte-code dumps?")
set(FEATURE_SNAPSHOT_EXEC    OFF     CACHE BOOL   "Enable executing snapshot files?")
set(FEATURE_SNAPSHOT_SAVE    OFF     CACHE BOOL   "Enable saving snapshot files?")
set(FEATURE_SYSTEM_ALLOCATOR OFF     CACHE BOOL   "Enable system allocator?")
set(FEATURE_VALGRIND         OFF     CACHE BOOL   "Enable Valgrind support?")
set(FEATURE_VALGRIND_FREYA   OFF     CACHE BOOL   "Enable Valgrind-Freya support?")
set(MEM_HEAP_SIZE_KB         "512"   CACHE STRING "Size of memory heap, in kilobytes")

if(FEATURE_SYSTEM_ALLOCATOR)
  set(FEATURE_CPOINTER_32_BIT ON)
endif()

# Status messages
message(STATUS "FEATURE_CPOINTER_32_BIT   " ${FEATURE_CPOINTER_32_BIT})
message(STATUS "FEATURE_DEBUGGER          " ${FEATURE_DEBUGGER})
message(STATUS "FEATURE_DEBUGGER_PORT     " ${FEATURE_DEBUGGER_PORT})
message(STATUS "FEATURE_ERROR_MESSAGES    " ${FEATURE_ERROR_MESSAGES})
message(STATUS "FEATURE_JS_PARSER         " ${FEATURE_JS_PARSER})
message(STATUS "FEATURE_MEM_STATS         " ${FEATURE_MEM_STATS})
message(STATUS "FEATURE_MEM_STRESS_TEST   " ${FEATURE_MEM_STRESS_TEST})
message(STATUS "FEATURE_PARSER_DUMP       " ${FEATURE_PARSER_DUMP})
message(STATUS "FEATURE_PROFILE           " ${FEATURE_PROFILE})
message(STATUS "FEATURE_REGEXP_DUMP       " ${FEATURE_REGEXP_DUMP})
message(STATUS "FEATURE_SNAPSHOT_EXEC     " ${FEATURE_SNAPSHOT_EXEC})
message(STATUS "FEATURE_SNAPSHOT_SAVE     " ${FEATURE_SNAPSHOT_SAVE})
message(STATUS "FEATURE_SYSTEM_ALLOCATOR  " ${FEATURE_SYSTEM_ALLOCATOR})
message(STATUS "FEATURE_VALGRIND          " ${FEATURE_VALGRIND})
message(STATUS "FEATURE_VALGRIND_FREYA    " ${FEATURE_VALGRIND_FREYA})
message(STATUS "MEM_HEAP_SIZE_KB          " ${MEM_HEAP_SIZE_KB})

# Include directories
set(INCLUDE_CORE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/debugger"
    "${CMAKE_CURRENT_SOURCE_DIR}/ecma/base"
    "${CMAKE_CURRENT_SOURCE_DIR}/ecma/builtin-objects"
    "${CMAKE_CURRENT_SOURCE_DIR}/ecma/builtin-objects/typedarray"
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
file(GLOB SOURCE_CORE_API                       *.c)
file(GLOB SOURCE_CORE_DEBUGGER                  debugger/*.c)
file(GLOB SOURCE_CORE_ECMA_BASE                 ecma/base/*.c)
file(GLOB SOURCE_CORE_ECMA_BUILTINS             ecma/builtin-objects/*.c)
file(GLOB SOURCE_CORE_ECMA_BUILTINS_TYPEDARRAY  ecma/builtin-objects/typedarray/*.c)
file(GLOB SOURCE_CORE_ECMA_OPERATIONS           ecma/operations/*.c)
file(GLOB SOURCE_CORE_JCONTEXT                  jcontext/*.c)
file(GLOB SOURCE_CORE_JMEM                      jmem/*.c)
file(GLOB SOURCE_CORE_JRT                       jrt/*.c)
file(GLOB SOURCE_CORE_LIT                       lit/*.c)
file(GLOB SOURCE_CORE_PARSER_JS                 parser/js/*.c)
file(GLOB SOURCE_CORE_PARSER_REGEXP             parser/regexp/*.c)
file(GLOB SOURCE_CORE_VM                        vm/*.c)

set(SOURCE_CORE_FILES
    ${SOURCE_CORE_API}
    ${SOURCE_CORE_ECMA_BASE}
    ${SOURCE_CORE_ECMA_BUILTINS}
    ${SOURCE_CORE_ECMA_BUILTINS_TYPEDARRAY}
    ${SOURCE_CORE_ECMA_OPERATIONS}
    ${SOURCE_CORE_JCONTEXT}
    ${SOURCE_CORE_JMEM}
    ${SOURCE_CORE_JRT}
    ${SOURCE_CORE_LIT}
    ${SOURCE_CORE_PARSER_JS}
    ${SOURCE_CORE_PARSER_REGEXP}
    ${SOURCE_CORE_VM})

if(FEATURE_DEBUGGER)
  set(SOURCE_CORE_FILES ${SOURCE_CORE_FILES} ${SOURCE_CORE_DEBUGGER})
endif()

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

# build mode specific compile/link flags
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_NDEBUG)
endif()

# Jerry heap-section
if(DEFINED JERRY_HEAP_SECTION_ATTR)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_HEAP_SECTION_ATTR=${JERRY_HEAP_SECTION_ATTR})
endif()

# Checks the optional features
# Enable 32 bit cpointers
if(FEATURE_CPOINTER_32_BIT)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_CPOINTER_32_BIT)
endif()

# Fill error messages for builtin error objects
if(FEATURE_ERROR_MESSAGES)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_ENABLE_ERROR_MESSAGES)
endif()

# JS-Parser
if(FEATURE_JS_PARSER)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_JS_PARSER)
else()
  if(NOT FEATURE_SNAPSHOT_EXEC MATCHES ON)
    set(FEATURE_SNAPSHOT_EXEC ON)
    message(STATUS "JS-Parser has been disabled, snapshot is now on")
  endif()
  if(FEATURE_PARSER_DUMP)
    set(FEATURE_PARSER_DUMP OFF)
    message(STATUS "Parser byte-code dumps has been disabled")
  endif()
endif()

# Memory statistics
if(FEATURE_MEM_STATS)
  set(DEFINES_JERRY ${DEFINES_JERRY} JMEM_STATS)
endif()

# Enable debugger
if(FEATURE_DEBUGGER)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_DEBUGGER)
endif()

# Debugger port
set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_DEBUGGER_PORT=${FEATURE_DEBUGGER_PORT})

# Memory management stress-test mode
if(FEATURE_MEM_STRESS_TEST)
  set(DEFINES_JERRY ${DEFINES_JERRY} JMEM_GC_BEFORE_EACH_ALLOC)
endif()

# Parser byte-code dumps
if(FEATURE_PARSER_DUMP)
  set(DEFINES_JERRY ${DEFINES_JERRY} PARSER_DUMP_BYTE_CODE)
endif()

if (NOT IS_ABSOLUTE ${FEATURE_PROFILE})
  set(FEATURE_PROFILE "${CMAKE_CURRENT_SOURCE_DIR}/profiles/${FEATURE_PROFILE}.profile")
endif()

if(EXISTS ${FEATURE_PROFILE})
  FILE(READ "${FEATURE_PROFILE}" PROFILE_SETTINGS)
  STRING(REGEX REPLACE "^#.*$" "" PROFILE_SETTINGS "${PROFILE_SETTINGS}")
  STRING(REGEX REPLACE "[\r|\n]" ";" PROFILE_SETTINGS "${PROFILE_SETTINGS}")
  set(DEFINES_JERRY ${DEFINES_JERRY} ${PROFILE_SETTINGS})
else()
  MESSAGE(FATAL_ERROR "Profile file: '${FEATURE_PROFILE}' doesn't exist!")
endif()

if(JERRY_LIBC AND FEATURE_DEBUGGER)
  MESSAGE(FATAL_ERROR "This configuration is not supported. Please build against your system libc to enable the JerryScript debugger.")
endif()

# RegExp byte-code dumps
if(FEATURE_REGEXP_DUMP)
  set(DEFINES_JERRY ${DEFINES_JERRY} REGEXP_DUMP_BYTE_CODE)
endif()

# Snapshot exec
if(FEATURE_SNAPSHOT_EXEC)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_ENABLE_SNAPSHOT_EXEC)
endif()

# Snapshot save
if(FEATURE_SNAPSHOT_SAVE)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_ENABLE_SNAPSHOT_SAVE)
endif()

# Enable system allocator
if(FEATURE_SYSTEM_ALLOCATOR)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_SYSTEM_ALLOCATOR)
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

# Size of heap
math(EXPR MEM_HEAP_AREA_SIZE "${MEM_HEAP_SIZE_KB} * 1024")
set(DEFINES_JERRY ${DEFINES_JERRY} CONFIG_MEM_HEAP_AREA_SIZE=${MEM_HEAP_AREA_SIZE})

add_library(${JERRY_CORE_NAME} STATIC ${SOURCE_CORE})

target_compile_definitions(${JERRY_CORE_NAME} PUBLIC ${DEFINES_JERRY})
target_include_directories(${JERRY_CORE_NAME} PUBLIC ${INCLUDE_CORE})

set(JERRY_LIBS )

if(JERRY_LIBM)
  set(JERRY_LIBS ${JERRY_LIBS} jerry-libm)
  target_include_directories(${JERRY_CORE_NAME} SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/jerry-libm/include")
endif()

if(JERRY_LIBC)
  set(JERRY_LIBS ${JERRY_LIBS} jerry-libc)
  target_include_directories(${JERRY_CORE_NAME} SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/jerry-libc/include")
endif()

target_link_libraries(${JERRY_CORE_NAME} ${JERRY_LIBS})

separate_arguments(EXTERNAL_LINK_LIBS)
foreach(EXT_LIB ${EXTERNAL_LINK_LIBS})
  target_link_libraries(${JERRY_CORE_NAME} ${EXT_LIB})
endforeach()

install(TARGETS ${JERRY_CORE_NAME} DESTINATION lib)
install(FILES jerry-api.h jerryscript.h jerry-port.h DESTINATION include)
