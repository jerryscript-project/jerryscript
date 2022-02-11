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
cmake_minimum_required(VERSION 3.5)

# Set JerryScript variables
set(JERRY_ROOT_DIR ${PROJECT_DIR}/../../..)
set(JERRY_BUILD_DIR ${JERRY_ROOT_DIR}/build/jerry)
set(JERRY_INCLUDE_DIRS "${JERRY_ROOT_DIR}/jerry-ext/include" "${JERRY_ROOT_DIR}/jerry-core/include")
set(JERRY_HEAP_SIZE 20)

# Make compile flags separated by spaces
string(REPLACE ";" " " COMPILE_FLAGS "${COMPILE_OPTIONS};${C_COMPILE_OPTIONS}")

# Build JerryScript
execute_process(COMMAND ${JERRY_ROOT_DIR}/tools/build.py
                            --clean
		                    --builddir=${JERRY_BUILD_DIR}
                            --lto=OFF
                            --jerry-cmdline=OFF
                            --jerry-math=OFF
                            --amalgam=ON
                            --mem-heap=${JERRY_HEAP_SIZE}
                            --profile=es.next
                            --compile-flag=${COMPILE_FLAGS}
                            --toolchain=${CMAKE_TOOLCHAIN_FILE})

# Register JerryScript target application
idf_component_register(SRCS "jerry-main.c" "jerry-port.c" INCLUDE_DIRS ${JERRY_INCLUDE_DIRS})

add_library(jerry-core STATIC IMPORTED)
add_library(jerry-ext STATIC IMPORTED)
add_library(jerry-port STATIC IMPORTED)

set_target_properties(jerry-core PROPERTIES IMPORTED_LOCATION "${JERRY_BUILD_DIR}/lib/libjerry-core.a")
set_target_properties(jerry-ext PROPERTIES IMPORTED_LOCATION "${JERRY_BUILD_DIR}/lib/libjerry-ext.a")
set_target_properties(jerry-port PROPERTIES IMPORTED_LOCATION "${JERRY_BUILD_DIR}/lib/libjerry-port.a")

# jerry-core has espressif/main/jerry-port.c as dependency
target_link_libraries(jerry-core INTERFACE ${COMPONENT_LIB})
# jerry-main has jerry-core, jerry-ext, jerry-port as dependencies
target_link_libraries(${COMPONENT_LIB} INTERFACE jerry-core jerry-ext jerry-port)
