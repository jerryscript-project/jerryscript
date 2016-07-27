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

# Sources
# Jerry standalone
set(SOURCE_JERRY_STANDALONE_MAIN main-unix.c)

# Generage map file
if("${PLATFORM}" STREQUAL "DARWIN")
  set(MAP_FILE_FLAGS "-Xlinker -map")
else()
  set(MAP_FILE_FLAGS "-Xlinker -Map")
endif()

set(MAP_FILE_FLAGS "${MAP_FILE_FLAGS} -Xlinker jerry.map")
set(LINKER_FLAGS_COMMON "${LINKER_FLAGS_COMMON} ${MAP_FILE_FLAGS}")

# Disable static build
if(NOT ("${PLATFORM}" STREQUAL "DARWIN"))
  set(LINKER_FLAGS_STATIC "-static")
endif()

add_executable(${JERRY_NAME} ${SOURCE_JERRY_STANDALONE_MAIN})
set_property(TARGET ${JERRY_NAME}
             PROPERTY LINK_FLAGS "${LINKER_FLAGS_STATIC} ${LINKER_FLAGS_COMMON}")
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
