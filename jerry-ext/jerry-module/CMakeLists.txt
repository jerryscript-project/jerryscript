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
set(JERRYX_MODULE_NAME jerry-module)
project (${JERRYX_MODULE_NAME} C)

# Include directories
set(INCLUDE_MODULE "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(JERRYX_CONTEXT)
  set(DEFINES_MODULE "-DJERRYX_MODULE_HAVE_CONTEXT")
endif()

# Source directories
file(GLOB SOURCE_MODULE *.c)

add_library(${JERRYX_MODULE_NAME} STATIC ${SOURCE_MODULE})
if(JERRYX_CONTEXT)
  target_include_directories(${JERRYX_MODULE_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/jerry-ext/jerry-context/include")
endif()
target_include_directories(${JERRYX_MODULE_NAME} PRIVATE ${INCLUDE_MODULE})
target_include_directories(${JERRYX_MODULE_NAME} INTERFACE ${INCLUDE_MODULE})
target_include_directories(${JERRYX_MODULE_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/jerry-core")
target_include_directories(${JERRYX_MODULE_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/jerry-core/jmem")
target_include_directories(${JERRYX_MODULE_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/jerry-core/jrt")
if(NOT "${DEFINES_MODULE}" STREQUAL "")
  target_compile_definitions(${JERRYX_MODULE_NAME} PRIVATE "${DEFINES_MODULE}")
endif()

install(TARGETS ${JERRYX_MODULE_NAME} DESTINATION lib)
install(DIRECTORY ${INCLUDE_MODULE}/ DESTINATION include/jerry-module)
