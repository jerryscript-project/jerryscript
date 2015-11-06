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
set(CMAKE_SYSTEM_PROCESSOR i686)

find_program(CMAKE_C_COMPILER NAMES i686-linux-gnu-gcc i686-unknown-linux-gnu-gcc)
find_program(CMAKE_CXX_COMPILER NAMES i686-linux-gnu-g++ i686-unknown-linux-gnu-g++)
# FIXME: This could break cross compilation, when the strip is not for the target architecture
find_program(CMAKE_STRIP NAMES i686-linux-gnu-strip i686-unknown-linux-gnu-strip strip)
