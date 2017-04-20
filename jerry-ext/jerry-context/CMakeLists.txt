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
set(JERRYX_CONTEXT_NAME jerry-context)
project (${JERRYX_CONTEXT_NAME} C)

set(JERRYX_CONTEXT_SLOTS "1" CACHE STRING "Number of context slots")

# Status messages
message(STATUS "JERRYX_CONTEXT_SLOTS      " ${JERRYX_CONTEXT_SLOTS})

# Include directories
set(INCLUDE_CONTEXT "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(DEFINES_CONTEXT "")

if(NOT "${JERRYX_CONTEXT_SLOTS}" STREQUAL "")
  set(DEFINES_CONTEXT ${DEFINES_CONTEXT} "-DJERRYX_CONTEXT_SLOTS=${JERRYX_CONTEXT_SLOTS}")
endif()

# Source directories
file(GLOB SOURCE_CONTEXT *.c)

add_library(${JERRYX_CONTEXT_NAME} STATIC ${SOURCE_CONTEXT})
set_property(TARGET ${JERRYX_CONTEXT_NAME}
             PROPERTY COMPILE_FLAGS "${DEFINES_CONTEXT}")

target_include_directories(${JERRYX_CONTEXT_NAME} PRIVATE ${INCLUDE_CONTEXT})
target_include_directories(${JERRYX_CONTEXT_NAME} INTERFACE ${INCLUDE_CONTEXT})
target_include_directories(${JERRYX_CONTEXT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/jerry-core")
target_include_directories(${JERRYX_CONTEXT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/jerry-core/jmem")
target_include_directories(${JERRYX_CONTEXT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/jerry-core/jrt")

install(TARGETS ${JERRYX_CONTEXT_NAME} DESTINATION lib)
install(DIRECTORY ${INCLUDE_CONTEXT}/ DESTINATION include/jerry-context)
