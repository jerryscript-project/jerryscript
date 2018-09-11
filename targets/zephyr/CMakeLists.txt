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
cmake_minimum_required(VERSION 3.8)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_sources(app PRIVATE src/main-zephyr.c src/jerry-port.c src/getline-zephyr.c)

add_library(jerry-core STATIC IMPORTED)
add_library(jerry-ext STATIC IMPORTED)
set_target_properties(jerry-core PROPERTIES IMPORTED_LOCATION
                      ${CMAKE_CURRENT_BINARY_DIR}/../obj/lib/libjerry-core.a)
set_target_properties(jerry-core PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                      ${CMAKE_CURRENT_SOURCE_DIR}/../../jerry-core/include)
set_target_properties(jerry-ext PROPERTIES IMPORTED_LOCATION
                      ${CMAKE_CURRENT_BINARY_DIR}/../obj/lib/libjerry-ext.a)
set_target_properties(jerry-ext PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                      ${CMAKE_CURRENT_SOURCE_DIR}/../../jerry-ext/include)
target_link_libraries(app PUBLIC jerry-core jerry-ext)

zephyr_get_include_directories_for_lang_as_string(C includes)
zephyr_get_system_include_directories_for_lang_as_string(C system_includes)
zephyr_get_compile_definitions_for_lang_as_string(C definitions)
zephyr_get_compile_options_for_lang_as_string(C options)

add_custom_target(
  outputexports
  COMMAND echo CC="${CMAKE_C_COMPILER}"
  COMMAND echo Z_CFLAGS=${system_includes} ${includes} ${definitions} ${options}
  COMMAND echo NOSTDINC_FLAGS=${system_includes}
  COMMAND echo ZEPHYRINCLUDE=${includes}
  COMMAND echo KBUILD_CFLAGS=${definitions} ${options}
  VERBATIM
  USES_TERMINAL
)

