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

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

set(JERRY_BASE ${CMAKE_SOURCE_DIR}/../../..)

# Build JerryScript
execute_process(COMMAND ${JERRY_BASE}/tools/build.py
                            --clean
                            --lto=OFF
                            --jerry-cmdline=OFF
                            --jerry-math=ON
                            --amalgam=ON
                            --mem-heap=70
                            --profile=es.next
                            --toolchain=${JERRY_BASE}/cmake/toolchain_mcu_stm32f4.cmake)

# Define library targets
add_library(jerry-core STATIC IMPORTED)
add_library(jerry-ext STATIC IMPORTED)
add_library(jerry-port STATIC IMPORTED)

# Define include directories and archives
set_target_properties(jerry-core PROPERTIES IMPORTED_LOCATION ${JERRY_BASE}/build/lib/libjerry-core.a)
set_target_properties(jerry-core PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${JERRY_BASE}/jerry-core/include)
set_target_properties(jerry-ext PROPERTIES IMPORTED_LOCATION ${JERRY_BASE}/build/lib/libjerry-ext.a)
set_target_properties(jerry-ext PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${JERRY_BASE}/jerry-ext/include)
set_target_properties(jerry-port PROPERTIES IMPORTED_LOCATION ${JERRY_BASE}/build/lib/libjerry-port.a)

target_sources(app PRIVATE src/jerry-main.c src/getline-zephyr.c src/jerry-port.c)
target_link_libraries(app PUBLIC jerry-core jerry-ext jerry-port)
