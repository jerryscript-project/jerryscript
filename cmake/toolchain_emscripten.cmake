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

include(CMakeForceCompiler)

set(CMAKE_C_COMPILER emcc)

set(CMAKE_EXECUTABLE_SUFFIX_C ".js")

set(FLAGS_COMMON_ARCH -s ERROR_ON_UNDEFINED_SYMBOLS=1 -s RESERVED_FUNCTION_POINTERS=1000 -s SAFE_HEAP=1 -s ASSERTIONS=1)
