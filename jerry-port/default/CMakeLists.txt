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
set(JERRY_PORT_DEFAULT_NAME jerry-port-default)
project (${JERRY_PORT_DEFAULT_NAME} C)

# Include directories
set(INCLUDE_PORT_DEFAULT "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Source directories
file(GLOB SOURCE_PORT_DEFAULT *.c)

# Define _BSD_SOURCE and _DEFAULT_SOURCE
# (should only be necessary if we used compiler default libc but not checking that)
set(DEFINES_PORT_DEFAULT _BSD_SOURCE _DEFAULT_SOURCE)

# Default Jerry port implementation library variants:
#   - default
#   - default-minimal (no extra termination and log APIs)
foreach(JERRY_PORT_LIBRARY_NAME ${JERRY_PORT_DEFAULT_NAME} ${JERRY_PORT_DEFAULT_NAME}-minimal)
  add_library(${JERRY_PORT_LIBRARY_NAME} STATIC ${SOURCE_PORT_DEFAULT})
  target_include_directories(${JERRY_PORT_LIBRARY_NAME} PUBLIC ${INCLUDE_PORT_DEFAULT})
  target_include_directories(${JERRY_PORT_LIBRARY_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/jerry-core/include)
  target_include_directories(${JERRY_PORT_LIBRARY_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/jerry-ext/include)
  target_compile_definitions(${JERRY_PORT_LIBRARY_NAME} PRIVATE ${DEFINES_PORT_DEFAULT})
  target_link_libraries(${JERRY_PORT_LIBRARY_NAME} jerry-core) # FIXME: remove this dependency as soon as possible
endforeach()

target_compile_definitions(${JERRY_PORT_DEFAULT_NAME}-minimal PRIVATE DISABLE_EXTRA_API)

# Installation
install(TARGETS ${JERRY_PORT_DEFAULT_NAME} ${JERRY_PORT_DEFAULT_NAME}-minimal DESTINATION lib)
install(DIRECTORY ${INCLUDE_PORT_DEFAULT}/ DESTINATION include)
