# Copyright 2015 Samsung Electronics Co., Ltd.
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

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR armv7l-hf)

set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)
# FIXME: This could break cross compilation, when the strip is not for the target architecture
find_program(CMAKE_STRIP NAMES arm-linux-gnueabihf-strip strip)
#
# Limit fpu to VFPv3 with d0-d15 registers
#
# If this is changed, setjmp / longjmp for ARMv7 should be updated accordingly
#
set(FLAGS_COMMON_ARCH -mlittle-endian -mthumb -mfpu=vfpv3-d16)
