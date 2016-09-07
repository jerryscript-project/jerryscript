# Copyright 2016 Samsung Electronics Co., Ltd.
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
set(JERRY_NAME jerry)
project (${JERRY_NAME} C)

# Optional build settings
set(ENABLE_LINK_MAP    OFF CACHE BOOL "Enable generating a link map file?")
set(ENABLE_STATIC_LINK ON  CACHE BOOL "Enable static linking?")

if("${PLATFORM}" STREQUAL "DARWIN")
  set(ENABLE_STATIC_LINK "OFF")
endif()

# Status messages
message(STATUS "ENABLE_LINK_MAP           " ${ENABLE_LINK_MAP})
message(STATUS "ENABLE_STATIC_LINK        " ${ENABLE_STATIC_LINK})

# Sources
# Jerry standalone
set(SOURCE_JERRY_STANDALONE_MAIN main-unix.c)

# Generate map file
if(ENABLE_LINK_MAP)
  if("${PLATFORM}" STREQUAL "DARWIN")
    set(LINKER_FLAGS_COMMON "${LINKER_FLAGS_COMMON} -Xlinker -map -Xlinker jerry.map")
  else()
    set(LINKER_FLAGS_COMMON "${LINKER_FLAGS_COMMON} -Xlinker -Map -Xlinker jerry.map")
  endif()
endif()

# Disable static build
if(ENABLE_STATIC_LINK)
  set(LINKER_FLAGS_COMMON "-static ${LINKER_FLAGS_COMMON}")
endif()

# Get version information from git
if(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/.git")
  execute_process(COMMAND git rev-parse --short HEAD
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                  OUTPUT_VARIABLE JERRY_COMMIT_HASH
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(JERRY_COMMIT_HASH " (${JERRY_COMMIT_HASH})")
else()
  set(JERRY_COMMIT_HASH "")
endif()

set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_COMMIT_HASH="${JERRY_COMMIT_HASH}")

add_executable(${JERRY_NAME} ${SOURCE_JERRY_STANDALONE_MAIN})
set_property(TARGET ${JERRY_NAME}
             PROPERTY LINK_FLAGS "${LINKER_FLAGS_COMMON}")
target_compile_definitions(${JERRY_NAME} PRIVATE ${DEFINES_JERRY})
target_include_directories(${JERRY_NAME} PRIVATE ${PORT_DIR})
link_directories(${CMAKE_BINARY_DIR})

set(JERRY_LIBS jerry-core)

if(JERRY_LIBM)
  set(JERRY_LIBS ${JERRY_LIBS} jerry-libm)
endif()

if(JERRY_LIBC)
  set(JERRY_LIBS ${JERRY_LIBS} jerry-libc)
endif()

set(JERRY_LIBS ${JERRY_LIBS} ${IMPORTED_LIB})

target_link_libraries(${JERRY_NAME} ${JERRY_LIBS})
