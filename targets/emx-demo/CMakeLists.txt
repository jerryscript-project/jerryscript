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
project (emx-demo C)

file(GLOB_RECURSE EMX_DEMO_SRCS **.c)

set(TARGET_NAME emx_demo)

add_executable(${TARGET_NAME} ${EMX_DEMO_SRCS})

set(EMX_DEMO_LINK_FLAGS "${EMX_DEMO_LINK_FLAGS} -s EXPORTED_FUNCTIONS=\"['_main', '_js_runtime_call_on_uart']\"")
set(EMX_DEMO_LINK_FLAGS "${EMX_DEMO_LINK_FLAGS} -g4 -O0")

set_property(TARGET ${TARGET_NAME}
        PROPERTY LINK_FLAGS "${LINKER_FLAGS_COMMON} ${EMX_DEMO_LINK_FLAGS}")
target_link_libraries(${TARGET_NAME} jerry-core jerry-ext jerry-port-default-minimal)
target_compile_options(${TARGET_NAME} PRIVATE -Wno-unused-variable -Wno-unused-parameter -Wno-dollar-in-identifier-extension -g4 -O0)

file(COPY browser/index.html DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(COPY browser/style.css DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(COPY browser/browser-ui.js DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(COPY browser/app.js DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
