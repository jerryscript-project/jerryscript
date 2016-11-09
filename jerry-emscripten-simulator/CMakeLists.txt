# Copyright 2016 Pebble Technology Corp.
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
set(JERRY_CORE_SIMULATOR_NAME jerry-core)
project (${JERRY_CORE_SIMULATOR_NAME} C)

set(INCLUDE_CORE "${CMAKE_SOURCE_DIR}/jerry-core")

file(GLOB SOURCE_JERRY_EMSCRIPTEN_SIMULATOR *.c)
file(GLOB SOURCE_PORT_FILES "${PORT_DIR}/*.c")
set(SOURCE_CORE ${SOURCE_JERRY_EMSCRIPTEN_SIMULATOR} ${SOURCE_PORT_FILES})

jerry_add_compile_warnings(no-dollar-in-identifier-extension)

add_library(${JERRY_CORE_SIMULATOR_NAME} STATIC ${SOURCE_CORE})

# Provides stdin through Node.js' process.stdin:
file(GLOB MODULE_JS module.js)

file(GLOB JERRY_JS jerry.js)

# Adds global.print() function:
file(GLOB PRINT_JS print.js)

# Work-around for https://public.kitware.com/Bug/view.php?id=15826
set(PRE_JS_OPTIONS "--pre-js ${MODULE_JS}" "--pre-js ${JERRY_JS}" "--pre-js ${PRINT_JS}")

target_compile_definitions(${JERRY_CORE_SIMULATOR_NAME} PUBLIC ${DEFINES_JERRY})
target_include_directories(${JERRY_CORE_SIMULATOR_NAME} PUBLIC ${INCLUDE_CORE})
target_link_libraries(${JERRY_CORE_SIMULATOR_NAME} ${EXTERNAL_LINK_LIBS} ${PRE_JS_OPTIONS})

install(TARGETS ${JERRY_CORE_SIMULATOR_NAME} DESTINATION lib)
install(FILES "${INCLUDE_CORE}/jerry-api.h" "${INCLUDE_CORE}/jerry-port.h" DESTINATION include)
