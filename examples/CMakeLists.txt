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
project (Examples C)

macro(jerry_create_executable JERRY_NAME)
  add_executable(${JERRY_NAME} ${ARGN})
  set_property(TARGET ${JERRY_NAME}
               PROPERTY LINK_FLAGS "${LINKER_FLAGS_COMMON}")
  target_compile_definitions(${JERRY_NAME} PRIVATE ${DEFINES_JERRY})

  target_link_libraries(${JERRY_NAME} jerry-core)

  install(TARGETS ${JERRY_NAME} DESTINATION bin)
endmacro()

if(EXAMPLES)
  # Example 1
  jerry_create_executable("example1" "example1.c")
  target_link_libraries("example1" jerry-port-default)

  # Example 2
  jerry_create_executable("example2" "example2.c")
  target_link_libraries("example2" jerry-ext jerry-port-default)

  # Example 3
  jerry_create_executable("example3" "example3.c")
  target_link_libraries("example3" jerry-ext jerry-port-default)

  # Example 4
  jerry_create_executable("example4" "example4.c")
  target_link_libraries("example4" jerry-ext jerry-port-default)

  # Example 5
  jerry_create_executable("example5" "example5.c")
  target_link_libraries("example5" jerry-ext jerry-port-default)

  # Example 6
  jerry_create_executable("example6" "example6.c")
  target_link_libraries("example6" jerry-ext jerry-port-default)

  # Example 7
  jerry_create_executable("example7" "example7.c")
  target_link_libraries("example7" jerry-ext jerry-port-default)
endif()
