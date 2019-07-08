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

# Optional build settings
set(ENABLE_ALL_IN_ONE               OFF     CACHE BOOL   "Enable all-in-one build?")

# Optional features
set(JERRY_CPOINTER_32_BIT           OFF     CACHE BOOL   "Enable 32 bit compressed pointers?")
set(JERRY_DEBUGGER                  OFF     CACHE BOOL   "Enable JerryScript debugger?")
set(JERRY_ERROR_MESSAGES            OFF     CACHE BOOL   "Enable error messages?")
set(JERRY_EXTERNAL_CONTEXT          OFF     CACHE BOOL   "Enable external context?")
set(JERRY_PARSER                    ON      CACHE BOOL   "Enable javascript-parser?")
set(JERRY_LINE_INFO                 OFF     CACHE BOOL   "Enable line info?")
set(JERRY_LOGGING                   OFF     CACHE BOOL   "Enable logging?")
set(JERRY_MEM_STATS                 OFF     CACHE BOOL   "Enable memory statistics?")
set(JERRY_MEM_GC_BEFORE_EACH_ALLOC  OFF     CACHE BOOL   "Enable mem-stress test?")
set(JERRY_PARSER_DUMP_BYTE_CODE     OFF     CACHE BOOL   "Enable parser byte-code dumps?")
set(JERRY_PROFILE                   "es5.1" CACHE STRING "Use default or other profile?")
set(JERRY_REGEXP_STRICT_MODE        OFF     CACHE BOOL   "Enable regexp strict mode?")
set(JERRY_REGEXP_DUMP_BYTE_CODE     OFF     CACHE BOOL   "Enable regexp byte-code dumps?")
set(JERRY_SNAPSHOT_EXEC             OFF     CACHE BOOL   "Enable executing snapshot files?")
set(JERRY_SNAPSHOT_SAVE             OFF     CACHE BOOL   "Enable saving snapshot files?")
set(JERRY_SYSTEM_ALLOCATOR          OFF     CACHE BOOL   "Enable system allocator?")
set(JERRY_VALGRIND                  OFF     CACHE BOOL   "Enable Valgrind support?")
set(JERRY_VM_EXEC_STOP              OFF     CACHE BOOL   "Enable VM execution stopping?")
set(JERRY_GLOBAL_HEAP_SIZE          "(512)" CACHE STRING "Size of memory heap, in kilobytes")
set(JERRY_REGEXP_RECURSION_LIMIT    "0"     CACHE STRING "Limit of regexp recursion depth")
set(JERRY_CALL_STACK_LIMIT          "0"     CACHE STRING "Limit of function call recursion depth")

# Option overrides
if(USING_MSVC)
  set(ENABLE_ALL_IN_ONE ON) # FIXME: This should not be needed but right now it is. To be tracked down and followed up.

  set(ENABLE_ALL_IN_ONE_MESSAGE " (FORCED BY COMPILER)")
endif()

if(JERRY_SYSTEM_ALLOCATOR)
  set(JERRY_CPOINTER_32_BIT ON)

  set(JERRY_CPOINTER_32_BIT_MESSAGE " (FORCED BY SYSTEM ALLOCATOR)")
endif()

if (JERRY_GLOBAL_HEAP_SIZE GREATER 512)
  set(JERRY_CPOINTER_32_BIT ON)

  set(JERRY_CPOINTER_32_BIT_MESSAGE " (FORCED BY HEAP SIZE)")
endif()

if(NOT JERRY_PARSER)
  set(JERRY_SNAPSHOT_EXEC ON)
  set(JERRY_PARSER_DUMP   OFF)

  set(JERRY_SNAPSHOT_EXEC_MESSAGE " (FORCED BY DISABLED JS PARSER)")
  set(JERRY_PARSER_DUMP_MESSAGE   " (FORCED BY DISABLED JS PARSER)")
endif()

if(JERRY_CMDLINE_SNAPSHOT)
  set(JERRY_SNAPSHOT_SAVE ON)

  set(JERRY_SNAPSHOT_SAVE_MESSAGE " (FORCED BY SNAPSHOT TOOL)")
endif()

if(JERRY_MEM_STATS OR JERRY_PARSER_DUMP_BYTE_CODE OR JERRY_REGEXP_DUMP_BYTE_CODE)
  set(JERRY_LOGGING ON)

  set(JERRYRE_LOGGING_MESSAGE " (FORCED BY STATS OR DUMP)")
endif()

# Status messages
message(STATUS "ENABLE_ALL_IN_ONE              " ${ENABLE_ALL_IN_ONE} ${ENABLE_ALL_IN_ONE_MESSAGE})
message(STATUS "JERRY_CPOINTER_32_BIT          " ${JERRY_CPOINTER_32_BIT} ${JERRY_CPOINTER_32_BIT_MESSAGE})
message(STATUS "JERRY_DEBUGGER                 " ${JERRY_DEBUGGER})
message(STATUS "JERRY_ERROR_MESSAGES           " ${JERRY_ERROR_MESSAGES})
message(STATUS "JERRY_EXTERNAL_CONTEXT         " ${JERRY_EXTERNAL_CONTEXT})
message(STATUS "JERRY_PARSER                   " ${JERRY_PARSER})
message(STATUS "JERRY_LINE_INFO                " ${JERRY_LINE_INFO})
message(STATUS "JERRY_LOGGING                  " ${JERRY_LOGGING} ${JERRY_LOGGING_MESSAGE})
message(STATUS "JERRY_MEM_STATS                " ${JERRY_MEM_STATS})
message(STATUS "JERRY_MEM_GC_BEFORE_EACH_ALLOC " ${JERRY_MEM_GC_BEFORE_EACH_ALLOC})
message(STATUS "JERRY_PARSER_DUMP_BYTE_CODE    " ${JERRY_PARSER_DUMP_BYTE_CODE} ${JERRY_PARSER_DUMP_MESSAGE})
message(STATUS "JERRY_PROFILE                  " ${JERRY_PROFILE})
message(STATUS "JERRY_REGEXP_STRICT_MODE       " ${JERRY_REGEXP_STRICT_MODE})
message(STATUS "JERRY_REGEXP_DUMP_BYTE_CODE    " ${JERRY_REGEXP_DUMP_BYTE_CODE})
message(STATUS "JERRY_SNAPSHOT_EXEC            " ${JERRY_SNAPSHOT_EXEC} ${JERRY_SNAPSHOT_EXEC_MESSAGE})
message(STATUS "JERRY_SNAPSHOT_SAVE            " ${JERRY_SNAPSHOT_SAVE} ${JERRY_SNAPSHOT_SAVE_MESSAGE})
message(STATUS "JERRY_SYSTEM_ALLOCATOR         " ${JERRY_SYSTEM_ALLOCATOR})
message(STATUS "JERRY_VALGRIND                 " ${JERRY_VALGRIND})
message(STATUS "JERRY_VM_EXEC_STOP             " ${JERRY_VM_EXEC_STOP})
message(STATUS "JERRY_GLOBAL_HEAP_SIZE         " ${JERRY_GLOBAL_HEAP_SIZE})
message(STATUS "JERRY_REGEXP_RECURSION_LIMIT   " ${JERRY_REGEXP_RECURSION_LIMIT})
message(STATUS "JERRY_CALL_STACK_LIMIT         " ${JERRY_CALL_STACK_LIMIT})

# Include directories
set(INCLUDE_CORE_PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(INCLUDE_CORE_PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/api"
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

set(INCLUDE_CORE_PUBLIC ${INCLUDE_CORE_PUBLIC} PARENT_SCOPE) # for jerry-port
set(INCLUDE_CORE_PRIVATE ${INCLUDE_CORE_PRIVATE} PARENT_SCOPE) # for tests/unit-core

# Sources
# Jerry core
file(GLOB SOURCE_CORE_FILES
     api/*.c
     debugger/*.c
     ecma/base/*.c
     ecma/builtin-objects/*.c
     ecma/builtin-objects/typedarray/*.c
     ecma/operations/*.c
     jcontext/*.c
     jmem/*.c
     jrt/*.c
     lit/*.c
     parser/js/*.c
     parser/regexp/*.c
     vm/*.c)

# All-in-one build
if(ENABLE_ALL_IN_ONE)
  set(ALL_IN_FILE "${CMAKE_BINARY_DIR}/jerry-all-in.c")
  list(SORT SOURCE_CORE_FILES)
  file(REMOVE ${ALL_IN_FILE})

  foreach(FILE ${SOURCE_CORE_FILES})
    file(APPEND ${ALL_IN_FILE} "#include \"${FILE}\"\n")
  endforeach()

  set(SOURCE_CORE_FILES ${ALL_IN_FILE})
endif()

# "Single" JerryScript source/header build.
#  The process will create the following files:
#   * jerryscript.c
#   * jerryscript.h
#   * jerryscript-config.h
if(ENABLE_ALL_IN_ONE_SOURCE)
  # Create a default configuration
  set(JERRYSCRIPT_CONFIG_H "${CMAKE_BINARY_DIR}/jerryscript-config.h")
  set(JERRYSCRIPT_SOURCE_CONFIG_H "${CMAKE_CURRENT_SOURCE_DIR}/config.h")
  add_custom_command(OUTPUT ${JERRYSCRIPT_CONFIG_H}
                     COMMAND ${CMAKE_COMMAND} -E copy ${JERRYSCRIPT_SOURCE_CONFIG_H} ${JERRYSCRIPT_CONFIG_H}
                     DEPENDS ${JERRYSCRIPT_SOURCE_CONFIG_H})

  # Create single C file
  file(GLOB HEADER_CORE_FILES *.h)
  set(ALL_IN_FILE "${CMAKE_BINARY_DIR}/jerryscript.c")
  set(ALL_IN_FILE_H "${CMAKE_BINARY_DIR}/jerryscript.h")
  add_custom_command(OUTPUT ${ALL_IN_FILE}
                     COMMAND python ${CMAKE_SOURCE_DIR}/tools/srcmerger.py
                             --base-dir ${CMAKE_CURRENT_SOURCE_DIR}
                             --input ${CMAKE_CURRENT_SOURCE_DIR}/api/jerry.c
                             --output ${ALL_IN_FILE}
                             --append-c-files
                             --remove-include jerryscript.h
                             --remove-include jerryscript-port.h
                             --remove-include jerryscript-compiler.h
                             --remove-include jerryscript-core.h
                             --remove-include jerryscript-debugger.h
                             --remove-include jerryscript-debugger-transport.h
                             --remove-include jerryscript-port.h
                             --remove-include jerryscript-snapshot.h
                             --remove-include config.h
                             --push-include jerryscript.h
                    DEPENDS ${SOURCE_CORE_FILES} ${ALL_IN_FILE_H} ${JERRYSCRIPT_CONFIG_H}
  )
  add_custom_command(OUTPUT ${ALL_IN_FILE_H}
                     COMMAND python ${CMAKE_SOURCE_DIR}/tools/srcmerger.py
                             --base-dir ${CMAKE_CURRENT_SOURCE_DIR}
                             --input ${CMAKE_CURRENT_SOURCE_DIR}/include/jerryscript.h
                             --output ${ALL_IN_FILE_H}
                             --remove-include config.h
                             --push-include jerryscript-config.h
                     DEPENDS ${HEADER_CORE_FILES} ${JERRYSCRIPT_CONFIG_H}
  )
  add_custom_target(generate-single-source-jerry DEPENDS ${ALL_IN_FILE} ${ALL_IN_FILE_H})
  add_custom_target(generate-single-source DEPENDS generate-single-source-jerry)

  set(SOURCE_CORE_FILES ${ALL_IN_FILE} ${ALL_IN_FILE_H})
endif()

# Third-party
# Valgrind
set(INCLUDE_THIRD_PARTY_VALGRIND "${CMAKE_SOURCE_DIR}/third-party/valgrind")

# build mode specific compile/link flags
set(DEFINES_JERRY ${DEFINES_JERRY} $<$<NOT:$<CONFIG:Debug>>:JERRY_NDEBUG>)

# Jerry heap-section
if(DEFINED JERRY_ATTR_GLOBAL_HEAP)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_ATTR_GLOBAL_HEAP=${JERRY_ATTR_GLOBAL_HEAP})
endif()

# Helper macro to set 0/1 switch as Jerry Defines
macro(jerry_add_define01 NAME)
  if(${NAME})
    set(DEFINES_JERRY ${DEFINES_JERRY} ${NAME}=1)
  else()
    set(DEFINES_JERRY ${DEFINES_JERRY} ${NAME}=0)
  endif()
endmacro(jerry_add_define01)

# Checks the optional features
# Enable 32 bit cpointers
jerry_add_define01(JERRY_CPOINTER_32_BIT)

# Fill error messages for builtin error objects
jerry_add_define01(JERRY_ERROR_MESSAGES)

# Use external context instead of static one
jerry_add_define01(JERRY_EXTERNAL_CONTEXT)

# JS-Parser
jerry_add_define01(JERRY_PARSER)

# JS line info
jerry_add_define01(JERRY_LINE_INFO)

# Logging
jerry_add_define01(JERRY_LOGGING)

# Memory statistics
jerry_add_define01(JERRY_MEM_STATS)

# Enable debugger
jerry_add_define01(JERRY_DEBUGGER)

# Memory management stress-test mode
jerry_add_define01(JERRY_MEM_GC_BEFORE_EACH_ALLOC)

# Parser byte-code dumps
jerry_add_define01(JERRY_PARSER_DUMP_BYTE_CODE)

# Profile
if (NOT IS_ABSOLUTE ${JERRY_PROFILE})
  set(JERRY_PROFILE "${CMAKE_CURRENT_SOURCE_DIR}/profiles/${JERRY_PROFILE}.profile")
endif()

if(EXISTS ${JERRY_PROFILE})
  file(READ "${JERRY_PROFILE}" PROFILE_SETTINGS)
  string(REGEX REPLACE "^#.*$" "" PROFILE_SETTINGS "${PROFILE_SETTINGS}")
  string(REGEX REPLACE "[\r|\n]" ";" PROFILE_SETTINGS "${PROFILE_SETTINGS}")
  set(DEFINES_JERRY ${DEFINES_JERRY} ${PROFILE_SETTINGS})
else()
  message(FATAL_ERROR "Profile file: '${JERRY_PROFILE}' doesn't exist!")
endif()

# RegExp strict mode
jerry_add_define01(JERRY_REGEXP_STRICT_MODE)

# RegExp recursion depth limit
if(JERRY_REGEXP_RECURSION_LIMIT)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_REGEXP_RECURSION_LIMIT=${JERRY_REGEXP_RECURSION_LIMIT})
endif()

# Function call recursion depth limit
if(JERRY_CALL_STACK_LIMIT)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_CALL_STACK_LIMIT=${JERRY_CALL_STACK_LIMIT})
endif()

# RegExp byte-code dumps
jerry_add_define01(JERRY_REGEXP_DUMP_BYTE_CODE)

# Snapshot exec
jerry_add_define01(JERRY_SNAPSHOT_EXEC)

# Snapshot save
jerry_add_define01(JERRY_SNAPSHOT_SAVE)

# Enable system allocator
jerry_add_define01(JERRY_SYSTEM_ALLOCATOR)

# Valgrind
jerry_add_define01(JERRY_VALGRIND)
if(JERRY_VALGRIND)
  set(INCLUDE_CORE_PRIVATE ${INCLUDE_CORE_PRIVATE} ${INCLUDE_THIRD_PARTY_VALGRIND})
endif()

# Enable VM execution stopping
jerry_add_define01(JERRY_VM_EXEC_STOP)

# Size of heap
set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_GLOBAL_HEAP_SIZE=${JERRY_GLOBAL_HEAP_SIZE})

add_library(${JERRY_CORE_NAME} ${SOURCE_CORE_FILES})

target_compile_definitions(${JERRY_CORE_NAME} PUBLIC ${DEFINES_JERRY})
target_include_directories(${JERRY_CORE_NAME} PUBLIC ${INCLUDE_CORE_PUBLIC})
target_include_directories(${JERRY_CORE_NAME} PRIVATE ${INCLUDE_CORE_PRIVATE})

set(JERRY_CORE_PKGCONFIG_REQUIRES)
set(JERRY_CORE_PKGCONFIG_LIBS)

if(JERRY_LIBM)
  target_link_libraries(${JERRY_CORE_NAME} jerry-libm)
  set(JERRY_CORE_PKGCONFIG_REQUIRES libjerry-libm)
endif()

separate_arguments(EXTERNAL_LINK_LIBS)
foreach(EXT_LIB ${EXTERNAL_LINK_LIBS})
  target_link_libraries(${JERRY_CORE_NAME} ${EXT_LIB})
  set(JERRY_CORE_PKGCONFIG_LIBS "${JERRY_CORE_PKGCONFIG_LIBS} -l${EXT_LIB}")
endforeach()

configure_file(libjerry-core.pc.in libjerry-core.pc @ONLY)

install(TARGETS ${JERRY_CORE_NAME} DESTINATION lib)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libjerry-core.pc DESTINATION lib/pkgconfig)
install(DIRECTORY ${INCLUDE_CORE_PUBLIC}/ DESTINATION include)
