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
set(JERRY_LIBM_NAME jerry-libm)
project (${JERRY_LIBM_NAME} C)

# Compiler / linker flags
# TODO: Reduce the below list of warning/error disablings as much as possible
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_LIBM} -Wno-error=sign-compare")
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_LIBM} -Wno-error=sign-conversion")
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_LIBM} -Wno-sign-conversion")
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_LIBM} -Wno-sign-compare")
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_LIBM} -Wno-strict-aliasing")

# Include directories
set(INCLUDE_LIBM "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Source directories
file(GLOB SOURCE_LIBM *.c)

add_library(${JERRY_LIBM_NAME} STATIC ${SOURCE_LIBM})
set_property(TARGET ${JERRY_LIBM_NAME}
             PROPERTY COMPILE_FLAGS "${COMPILE_FLAGS_LIBM}")

target_include_directories(${JERRY_LIBM_NAME} PRIVATE ${INCLUDE_LIBM})
target_include_directories(${JERRY_LIBM_NAME} INTERFACE ${INCLUDE_LIBM})

install(TARGETS ${JERRY_LIBM_NAME} DESTINATION lib)
install(DIRECTORY ${INCLUDE_LIBM}/ DESTINATION include/jerry-libm)
