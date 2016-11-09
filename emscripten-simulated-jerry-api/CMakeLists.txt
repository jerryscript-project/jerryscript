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
set(JERRY_CORE_SIMULATOR_NAME jerry-core)
set(JERRY_EXT_SIMULATOR_NAME jerry-ext)

project (${JERRY_CORE_SIMULATOR_NAME} C)
project (${JERRY_EXT_SIMULATOR_NAME} C)
################################
# Jerry-core
###############################

# Include directories
set(INCLUDE_CORE
        "${CMAKE_CURRENT_SOURCE_DIR}/../jerry-core"
        "${CMAKE_CURRENT_SOURCE_DIR}/../jerry-core/api"
        "${CMAKE_CURRENT_SOURCE_DIR}/../jerry-core/debugger"
        "${CMAKE_CURRENT_SOURCE_DIR}/../jerry-core/ecma/base"
        "${CMAKE_CURRENT_SOURCE_DIR}/../jerry-core/ecma/builtin-objects"
        "${CMAKE_CURRENT_SOURCE_DIR}/../jerry-core/ecma/builtin-objects/typedarray"
        "${CMAKE_CURRENT_SOURCE_DIR}/../jerry-core/ecma/operations"
        "${CMAKE_CURRENT_SOURCE_DIR}/../jerry-core/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../jerry-core/jcontext"
        "${CMAKE_CURRENT_SOURCE_DIR}/../jerry-core/jmem"
        "${CMAKE_CURRENT_SOURCE_DIR}/../jerry-core/jrt"
        "${CMAKE_CURRENT_SOURCE_DIR}/../jerry-core/lit"
        "${CMAKE_CURRENT_SOURCE_DIR}/../jerry-core/parser/js"
        "${CMAKE_CURRENT_SOURCE_DIR}/../jerry-core/parser/regexp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../jerry-core/vm")


file(GLOB SOURCE_JERRY_EMSCRIPTEN_SIMULATOR *.c)
file(GLOB SOURCE_PORT_FILES "${PORT_DIR}/*.c")

# Include some of the actual C files of JerryScript:
set(SOURCE_JERRY_ORIG_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../jerry-core/lit/lit-strings.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jerry-core/jrt/jrt-fatals.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jerry-core/jcontext/jcontext.c)

set(SOURCE_CORE ${SOURCE_JERRY_EMSCRIPTEN_SIMULATOR} ${SOURCE_PORT_FILES} ${SOURCE_JERRY_ORIG_FILES})

jerry_add_compile_warnings(no-dollar-in-identifier-extension)

add_library(${JERRY_CORE_SIMULATOR_NAME} STATIC ${SOURCE_CORE})

# Provides stdin through Node.js' process.stdin:
file(GLOB MODULE_JS module.js)
file(GLOB PROMISE_JS es6-promise.js)
file(GLOB JERRY_JS jerry.js)

# Work-around for https://public.kitware.com/Bug/view.php?id=15826
set(EMCC_LINK_FLAGS
    "--pre-js ${MODULE_JS}"
    "--pre-js ${PROMISE_JS}"
    "--pre-js ${JERRY_JS}"
    "-s ERROR_ON_UNDEFINED_SYMBOLS=1"
    "-s SAFE_HEAP=1"
    "-s FORCE_FILESYSTEM=1"
    "--memory-init-file 0")

target_compile_definitions(${JERRY_CORE_SIMULATOR_NAME} PUBLIC ${DEFINES_JERRY})
target_include_directories(${JERRY_CORE_SIMULATOR_NAME} PUBLIC ${INCLUDE_CORE})
target_link_libraries(${JERRY_CORE_SIMULATOR_NAME} ${EXTERNAL_LINK_LIBS} ${EMCC_LINK_FLAGS})

set(JS_DEPENDS
    ${MODULE_JS}
    ${PROMISE_JS}
    ${JERRY_JS})
set_property(TARGET ${JERRY_CORE_SIMULATOR_NAME} PROPERTY LINK_DEPENDS ${JS_DEPENDS})

