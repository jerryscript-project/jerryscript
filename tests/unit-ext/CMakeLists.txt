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
project (unit-ext C)

set(INCLUDE_UNIT_EXT ${CMAKE_CURRENT_SOURCE_DIR})

# Unit tests main modules
set(SOURCE_UNIT_TEST_EXT_MODULES
  test-ext-arg.c
  test-ext-autorelease.c
  test-ext-handle-scope-escape.c
  test-ext-handle-scope-handle-prelist-escape.c
  test-ext-handle-scope-handle-prelist.c
  test-ext-handle-scope-nested.c
  test-ext-handle-scope-remove.c
  test-ext-handle-scope-root.c
  test-ext-handle-scope.c
  test-ext-method-register.c
  test-ext-module-canonical.c
  test-ext-module-empty.c
)

# Disable test-ext-autorelease.c if compiler is MSVC, because MSVC doesn't support cleanup attribute.
if(USING_MSVC)
  list(REMOVE_ITEM SOURCE_UNIT_TEST_EXT_MODULES test-ext-autorelease.c)
endif()

# Unit tests declaration
add_custom_target(unittests-ext)

foreach(SOURCE_UNIT_TEST_EXT ${SOURCE_UNIT_TEST_EXT_MODULES})
  get_filename_component(TARGET_NAME ${SOURCE_UNIT_TEST_EXT} NAME_WE)
  set(TARGET_NAME unit-${TARGET_NAME})

  add_executable(${TARGET_NAME} ${SOURCE_UNIT_TEST_EXT})
  set_property(TARGET ${TARGET_NAME} PROPERTY LINK_FLAGS "${LINKER_FLAGS_COMMON}")
  set_property(TARGET ${TARGET_NAME} PROPERTY RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")

  target_link_libraries(${TARGET_NAME} jerry-ext jerry-core jerry-port-default)

  add_dependencies(unittests-ext ${TARGET_NAME})
endforeach()

set(UNIT_TEST_EXT_DIRS
  module
)

foreach(UNIT_TEST_EXT_DIR ${UNIT_TEST_EXT_DIRS})
  add_subdirectory(${UNIT_TEST_EXT_DIR})
endforeach()
