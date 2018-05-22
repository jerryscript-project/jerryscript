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
set(JERRY_LIBC_NAME jerry-libc)
project (${JERRY_LIBC_NAME} C ASM)

# Checks the optional features
# Enable init/fini arrays
if(FEATURE_INIT_FINI)
  set(DEFINES_LIBC ${DEFINES_LIBC} ENABLE_INIT_FINI)
endif()

# Include directories
set(INCLUDE_LIBC "${CMAKE_CURRENT_SOURCE_DIR}")

# Sources
file(GLOB SOURCE_LIBC *.c)

# Platform-specific
# Linux
if(DEFINED PLATFORM AND ((PLATFORM STREQUAL "LINUX") OR (PLATFORM STREQUAL "DARWIN")))
  file(GLOB TARGET_SPECIFIC_LIBC_SOURCE target/posix/*.c target/posix/*.S)
endif()

add_library(${JERRY_LIBC_NAME} ${SOURCE_LIBC} ${TARGET_SPECIFIC_LIBC_SOURCE})

target_compile_definitions(${JERRY_LIBC_NAME} PRIVATE ${DEFINES_LIBC})
target_include_directories(${JERRY_LIBC_NAME} PRIVATE ${INCLUDE_LIBC})
target_include_directories(${JERRY_LIBC_NAME} SYSTEM PUBLIC "${CMAKE_SOURCE_DIR}/jerry-libc/include")
target_link_libraries(${JERRY_LIBC_NAME} -lgcc)

install(TARGETS ${JERRY_LIBC_NAME} DESTINATION lib)
install(DIRECTORY ${INCLUDE_LIBC}/include/ DESTINATION include/jerry-libc)
