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
project (unit-core C)

if (NOT IS_ABSOLUTE ${FEATURE_PROFILE})
  set(FEATURE_PROFILE "${CMAKE_SOURCE_DIR}/jerry-core/profiles/${FEATURE_PROFILE}.profile")
endif()

if(${FEATURE_PROFILE} STREQUAL "${CMAKE_SOURCE_DIR}/jerry-core/profiles/minimal.profile")
  message(FATAL_ERROR "minimal profile isn't supported in unit-core")
endif()

# Unit tests main modules
file(GLOB SOURCE_UNIT_TEST_MAIN_MODULES *.c)

# Unit tests declaration
add_custom_target(unittests-core)

foreach(SOURCE_UNIT_TEST_MAIN ${SOURCE_UNIT_TEST_MAIN_MODULES})
  get_filename_component(TARGET_NAME ${SOURCE_UNIT_TEST_MAIN} NAME_WE)
  set(TARGET_NAME unit-${TARGET_NAME})

  add_executable(${TARGET_NAME} ${SOURCE_UNIT_TEST_MAIN})
  set_property(TARGET ${TARGET_NAME}
               PROPERTY LINK_FLAGS "${LINKER_FLAGS_COMMON}")

  target_link_libraries(${TARGET_NAME} jerry-core jerry-port-default-minimal)

  add_dependencies(unittests-core ${TARGET_NAME})
endforeach()
