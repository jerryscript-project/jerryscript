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
set(EMSCRIPTEN_SNAPSHOT_COMPILER jerryscript-snapshot-compiler)
project (${EMSCRIPTEN_SNAPSHOT_COMPILER} C)

file(GLOB PRE_JS pre.js)
file(GLOB POST_JS post.js)

# Work-around for https://public.kitware.com/Bug/view.php?id=15826
set(EMCC_LINK_FLAGS
        "--pre-js ${PRE_JS}"
        "--post-js ${POST_JS}"
        "-s ERROR_ON_UNDEFINED_SYMBOLS=1"
        "-s SAFE_HEAP=1"
        "-s EXPORTED_FUNCTIONS=\"['_emscripten_snapshot_compiler_get_version', '_emscripten_snapshot_compiler_compile']\""
        "-g4"
        "--memory-init-file 0")

jerry_create_executable(${EMSCRIPTEN_SNAPSHOT_COMPILER} "emscripten-snapshot-compiler.c")
target_link_libraries(${EMSCRIPTEN_SNAPSHOT_COMPILER} jerry-port-default ${EMCC_LINK_FLAGS})
set_target_properties(${EMSCRIPTEN_SNAPSHOT_COMPILER} PROPERTIES LINK_DEPENDS "${POST_JS};${PRE_JS}")

# In EM_ASM macros, C arguments are named $0, $1, etc.
target_compile_options(${EMSCRIPTEN_SNAPSHOT_COMPILER} PRIVATE "-Wno-dollar-in-identifier-extension")
