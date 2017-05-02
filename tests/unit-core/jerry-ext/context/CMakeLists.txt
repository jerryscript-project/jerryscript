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
set(JERRYX_CONTEXT_UNITTEST_NAME unit-test-jerry-context)

project (${JERRYX_CONTEXT_UNITTEST_NAME} C)

file(GLOB JERRYX_CONTEXT_UNIT_TEST_SOURCES *.c)

add_executable(${JERRYX_CONTEXT_UNITTEST_NAME} ${JERRYX_CONTEXT_UNIT_TEST_SOURCES})
set_property(TARGET ${JERRYX_CONTEXT_UNITTEST_NAME} PROPERTY LINK_FLAGS "${LINKER_FLAGS_COMMON}")
target_link_libraries(${JERRYX_CONTEXT_UNITTEST_NAME} jerryx-context jerry-core jerry-port-default-minimal)
target_include_directories(${JERRYX_CONTEXT_UNITTEST_NAME} PRIVATE ${INCLUDE_UNITTESTS})
target_include_directories(${JERRYX_CONTEXT_UNITTEST_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/jerry-ext/context/include")
