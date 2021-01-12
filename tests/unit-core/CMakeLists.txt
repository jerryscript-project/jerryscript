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
set(SOURCE_UNIT_TEST_MAIN_MODULES
  test-has-property.c
  test-api-objecttype.c
  test-strings.c
  test-exec-stop.c
  test-api-functiontype.c
  test-external-string.c
  test-from-property-descriptor.c
  test-dataview.c
  test-api-errortype.c
  test-arraybuffer.c
  test-api-object-property-names.c
  test-to-integer.c
  test-regression-3588.c
  test-number-converter.c
  test-api-value-type.c
  test-jmem.c
  test-promise.c
  test-realm.c
  test-internal-properties.c
  test-abort.c
  test-api-promise.c
  test-context-data.c
  test-backtrace.c
  test-objects-foreach.c
  test-api-binary-operations-arithmetics.c
  test-api-binary-operations-comparisons.c
  test-to-length.c
  test-api-strings.c
  test-proxy.c
  test-container.c
  test-api-binary-operations-instanceof.c
  test-snapshot.c
  test-native-callback-nested.c
  test-lit-char-helpers.c
  test-resource-name.c
  test-unicode.c
  test-bigint.c
  test-api-property.c
  test-api-iteratortype.c
  test-api.c
  test-typedarray.c
  test-api-set-and-clear-error-flag.c
  test-json.c
  test-native-instanceof.c
  test-literal-storage.c
  test-mem-stats.c
  test-newtarget.c
  test-number-to-int32.c
  test-number-to-string.c
  test-string-to-number.c
  test-poolman.c
  test-regexp-dotall-unicode.c
  test-date-helpers.c
  test-regexp.c
  test-stringbuilder.c
  test-symbol.c
  test-to-property-descriptor.c
)

# jerry_heap_stats_t.size == 0 if system allocator is used.
if(JERRY_SYSTEM_ALLOCATOR)
  list(REMOVE_ITEM SOURCE_UNIT_TEST_MAIN_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/test-mem-stats.c)
endif()

# Unit tests declaration
add_custom_target(unittests-core)

foreach(SOURCE_UNIT_TEST_MAIN ${SOURCE_UNIT_TEST_MAIN_MODULES})
  get_filename_component(TARGET_NAME ${SOURCE_UNIT_TEST_MAIN} NAME_WE)
  set(TARGET_NAME unit-${TARGET_NAME})

  add_executable(${TARGET_NAME} ${SOURCE_UNIT_TEST_MAIN})
  target_include_directories(${TARGET_NAME} PRIVATE ${INCLUDE_CORE_PRIVATE})
  set_property(TARGET ${TARGET_NAME} PROPERTY LINK_FLAGS "${LINKER_FLAGS_COMMON}")
  set_property(TARGET ${TARGET_NAME} PROPERTY RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
  target_link_libraries(${TARGET_NAME} jerry-core jerry-port-default)

  add_dependencies(unittests-core ${TARGET_NAME})
endforeach()
