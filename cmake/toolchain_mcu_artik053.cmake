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

set(CMAKE_SYSTEM_NAME TizenRT)
set(CMAKE_SYSTEM_PROCESSOR armv7l)
set(CMAKE_SYSTEM_VERSION ARTIK053)

set(FLAGS_COMMON_ARCH -mcpu=cortex-r4 -mthumb -mfpu=vfpv3
                      -fno-builtin -fno-strict-aliasing -fomit-frame-pointer -fno-strength-reduce
                      -Wall -Werror -Wshadow -Wno-error=conversion)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_C_COMPILER_WORKS TRUE)
