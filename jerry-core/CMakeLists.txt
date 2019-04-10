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
set(FEATURE_CPOINTER_32_BIT         OFF     CACHE BOOL   "Enable 32 bit compressed pointers?")
set(FEATURE_DEBUGGER                OFF     CACHE BOOL   "Enable JerryScript debugger?")
set(FEATURE_ERROR_MESSAGES          OFF     CACHE BOOL   "Enable error messages?")
set(FEATURE_EXTERNAL_CONTEXT        OFF     CACHE BOOL   "Enable external context?")
set(FEATURE_JS_PARSER               ON      CACHE BOOL   "Enable js-parser?")
set(FEATURE_LINE_INFO               OFF     CACHE BOOL   "Enable line info?")
set(FEATURE_LOGGING                 OFF     CACHE BOOL   "Enable logging?")
set(FEATURE_MEM_STATS               OFF     CACHE BOOL   "Enable memory statistics?")
set(FEATURE_MEM_STRESS_TEST         OFF     CACHE BOOL   "Enable mem-stress test?")
set(FEATURE_PARSER_DUMP             OFF     CACHE BOOL   "Enable parser byte-code dumps?")
set(FEATURE_PROFILE                 "es5.1" CACHE STRING "Use default or other profile?")
set(FEATURE_REGEXP_STRICT_MODE      OFF     CACHE BOOL   "Enable regexp strict mode?")
set(FEATURE_REGEXP_DUMP             OFF     CACHE BOOL   "Enable regexp byte-code dumps?")
set(FEATURE_SNAPSHOT_EXEC           OFF     CACHE BOOL   "Enable executing snapshot files?")
set(FEATURE_SNAPSHOT_SAVE           OFF     CACHE BOOL   "Enable saving snapshot files?")
set(FEATURE_SYSTEM_ALLOCATOR        OFF     CACHE BOOL   "Enable system allocator?")
set(FEATURE_VALGRIND                OFF     CACHE BOOL   "Enable Valgrind support?")
set(FEATURE_VM_EXEC_STOP            OFF     CACHE BOOL   "Enable VM execution stopping?")
set(JERRY_GLOBAL_HEAP_SIZE          "512"   CACHE STRING "Size of memory heap, in kilobytes")
set(JERRY_REGEXP_RECURSION_LIMIT    "0"     CACHE STRING "Limit of regexp recursion depth")
set(JERRY_VM_RECURSION_LIMIT        "0"     CACHE STRING "Limit of VM recursion depth")

# Option overrides
if(USING_MSVC)
  set(ENABLE_ALL_IN_ONE ON) # FIXME: This should not be needed but right now it is. To be tracked down and followed up.

  set(ENABLE_ALL_IN_ONE_MESSAGE " (FORCED BY COMPILER)")
endif()

if(FEATURE_SYSTEM_ALLOCATOR)
  set(FEATURE_CPOINTER_32_BIT ON)

  set(FEATURE_CPOINTER_32_BIT_MESSAGE " (FORCED BY SYSTEM ALLOCATOR)")
endif()

if (JERRY_GLOBAL_HEAP_SIZE GREATER 512)
  set(FEATURE_CPOINTER_32_BIT ON)

  set(FEATURE_CPOINTER_32_BIT_MESSAGE " (FORCED BY HEAP SIZE)")
endif()

if(NOT FEATURE_JS_PARSER)
  set(FEATURE_SNAPSHOT_EXEC ON)
  set(FEATURE_PARSER_DUMP   OFF)

  set(FEATURE_SNAPSHOT_EXEC_MESSAGE " (FORCED BY DISABLED JS PARSER)")
  set(FEATURE_PARSER_DUMP_MESSAGE   " (FORCED BY DISABLED JS PARSER)")
endif()

if(JERRY_CMDLINE_SNAPSHOT)
  set(FEATURE_SNAPSHOT_SAVE ON)

  set(FEATURE_SNAPSHOT_SAVE_MESSAGE " (FORCED BY SNAPSHOT TOOL)")
endif()

if(FEATURE_MEM_STATS OR FEATURE_PARSER_DUMP OR FEATURE_REGEXP_DUMP)
  set(FEATURE_LOGGING ON)

  set(FEATURE_LOGGING_MESSAGE " (FORCED BY STATS OR DUMP)")
endif()

# Status messages
message(STATUS "ENABLE_ALL_IN_ONE            " ${ENABLE_ALL_IN_ONE} ${ENABLE_ALL_IN_ONE_MESSAGE})
message(STATUS "FEATURE_CPOINTER_32_BIT      " ${FEATURE_CPOINTER_32_BIT} ${FEATURE_CPOINTER_32_BIT_MESSAGE})
message(STATUS "FEATURE_DEBUGGER             " ${FEATURE_DEBUGGER})
message(STATUS "FEATURE_ERROR_MESSAGES       " ${FEATURE_ERROR_MESSAGES})
message(STATUS "FEATURE_EXTERNAL_CONTEXT     " ${FEATURE_EXTERNAL_CONTEXT})
message(STATUS "FEATURE_JS_PARSER            " ${FEATURE_JS_PARSER})
message(STATUS "FEATURE_LINE_INFO            " ${FEATURE_LINE_INFO})
message(STATUS "FEATURE_LOGGING              " ${FEATURE_LOGGING} ${FEATURE_LOGGING_MESSAGE})
message(STATUS "FEATURE_MEM_STATS            " ${FEATURE_MEM_STATS})
message(STATUS "FEATURE_MEM_STRESS_TEST      " ${FEATURE_MEM_STRESS_TEST})
message(STATUS "FEATURE_PARSER_DUMP          " ${FEATURE_PARSER_DUMP} ${FEATURE_PARSER_DUMP_MESSAGE})
message(STATUS "FEATURE_PROFILE              " ${FEATURE_PROFILE})
message(STATUS "FEATURE_REGEXP_STRICT_MODE   " ${FEATURE_REGEXP_STRICT_MODE})
message(STATUS "FEATURE_REGEXP_DUMP          " ${FEATURE_REGEXP_DUMP})
message(STATUS "FEATURE_SNAPSHOT_EXEC        " ${FEATURE_SNAPSHOT_EXEC} ${FEATURE_SNAPSHOT_EXEC_MESSAGE})
message(STATUS "FEATURE_SNAPSHOT_SAVE        " ${FEATURE_SNAPSHOT_SAVE} ${FEATURE_SNAPSHOT_SAVE_MESSAGE})
message(STATUS "FEATURE_SYSTEM_ALLOCATOR     " ${FEATURE_SYSTEM_ALLOCATOR})
message(STATUS "FEATURE_VALGRIND             " ${FEATURE_VALGRIND})
message(STATUS "FEATURE_VM_EXEC_STOP         " ${FEATURE_VM_EXEC_STOP})
message(STATUS "JERRY_GLOBAL_HEAP_SIZE       " ${JERRY_GLOBAL_HEAP_SIZE})
message(STATUS "JERRY_REGEXP_RECURSION_LIMIT " ${JERRY_REGEXP_RECURSION_LIMIT})
message(STATUS "JERRY_VM_RECURSION_LIMIT     " ${JERRY_VM_RECURSION_LIMIT})

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

# Checks the optional features
# Enable 32 bit cpointers
if(FEATURE_CPOINTER_32_BIT)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_CPOINTER_32_BIT=1)
endif()

# Fill error messages for builtin error objects
if(FEATURE_ERROR_MESSAGES)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_ERROR_MESSAGES=1)
endif()

# Use external context instead of static one
if(FEATURE_EXTERNAL_CONTEXT)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_EXTERNAL_CONTEXT=1)
endif()

# JS-Parser
if(NOT FEATURE_JS_PARSER)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_PARSER=0)
endif()

# JS line info
if(FEATURE_LINE_INFO)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_LINE_INFO=1)
endif()

# Logging
if(FEATURE_LOGGING)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_LOGGING=1)
endif()

# Memory statistics
if(FEATURE_MEM_STATS)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_MEM_STATS=1)
endif()

# Enable debugger
if(FEATURE_DEBUGGER)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_DEBUGGER)
endif()

# Memory management stress-test mode
if(FEATURE_MEM_STRESS_TEST)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_MEM_GC_BEFORE_EACH_ALLOC=1)
endif()

# Parser byte-code dumps
if(FEATURE_PARSER_DUMP)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_PARSER_DUMP_BYTE_CODE=1)
endif()

# Profile
if (NOT IS_ABSOLUTE ${FEATURE_PROFILE})
  set(FEATURE_PROFILE "${CMAKE_CURRENT_SOURCE_DIR}/profiles/${FEATURE_PROFILE}.profile")
endif()

if(EXISTS ${FEATURE_PROFILE})
  file(READ "${FEATURE_PROFILE}" PROFILE_SETTINGS)
  string(REGEX REPLACE "^#.*$" "" PROFILE_SETTINGS "${PROFILE_SETTINGS}")
  string(REGEX REPLACE "[\r|\n]" ";" PROFILE_SETTINGS "${PROFILE_SETTINGS}")
  set(DEFINES_JERRY ${DEFINES_JERRY} ${PROFILE_SETTINGS})
else()
  message(FATAL_ERROR "Profile file: '${FEATURE_PROFILE}' doesn't exist!")
endif()

# RegExp strict mode
if(FEATURE_REGEXP_STRICT_MODE)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_REGEXP_STRICT_MODE=1)
endif()

# RegExp recursion depth limit
if(JERRY_REGEXP_RECURSION_LIMIT)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_REGEXP_RECURSION_LIMIT=${JERRY_REGEXP_RECURSION_LIMIT})
endif()

# VM recursion depth limit
if(JERRY_VM_RECURSION_LIMIT)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_VM_RECURSION_LIMIT=${JERRY_VM_RECURSION_LIMIT})
endif()

# RegExp byte-code dumps
if(FEATURE_REGEXP_DUMP)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_REGEXP_DUMP_BYTE_CODE=1)
endif()

# Snapshot exec
if(FEATURE_SNAPSHOT_EXEC)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_SNAPSHOT_EXEC=1)
endif()

# Snapshot save
if(FEATURE_SNAPSHOT_SAVE)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_SNAPSHOT_SAVE=1)
endif()

# Enable system allocator
if(FEATURE_SYSTEM_ALLOCATOR)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_SYSTEM_ALLOCATOR=1)
endif()

# Valgrind
if(FEATURE_VALGRIND)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_VALGRIND=1)
  set(INCLUDE_CORE_PRIVATE ${INCLUDE_CORE_PRIVATE} ${INCLUDE_THIRD_PARTY_VALGRIND})
endif()

# Enable VM execution stopping
if (FEATURE_VM_EXEC_STOP)
  set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_VM_EXEC_STOP=1)
endif()

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
