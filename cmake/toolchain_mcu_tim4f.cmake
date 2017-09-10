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

include(CMakeForceCompiler)

set(CMAKE_SYSTEM_NAME MCU)
set(CMAKE_SYSTEM_PROCESSOR armv7l)
set(CMAKE_SYSTEM_VERSION TIM4F)

set(FLAGS_COMMON_ARCH --little_endian --silicon_version=7M4 --float_support=FPv4SPD16)

CMAKE_FORCE_C_COMPILER(armcl TI)

SET (CMAKE_C_FLAGS_DEBUG_INIT          "-g")
SET (CMAKE_C_FLAGS_MINSIZEREL_INIT     "-o4 -mf0 -DNDEBUG")
SET (CMAKE_C_FLAGS_RELEASE_INIT        "-o4 -DNDEBUG")
SET (CMAKE_C_FLAGS_RELWITHDEBINFO_INIT "-o2 -g")

SET (CMAKE_CXX_FLAGS_DEBUG_INIT          "-g")
SET (CMAKE_CXX_FLAGS_MINSIZEREL_INIT     "-o4 -mf0 -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELEASE_INIT        "-o4 -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "-o2 -g")
