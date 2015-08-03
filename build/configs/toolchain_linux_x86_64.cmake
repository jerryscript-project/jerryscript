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
set(CMAKE_SYSTEM_PROCESSOR x86_64)

if(NOT DEFINED CMAKE_C_COMPILER)
  find_program(CMAKE_C_COMPILER NAMES x86_64-linux-gnu-gcc x86_64-unknown-linux-gnu-gcc)
endif()
if(NOT DEFINED CMAKE_CXX_COMPILER)
  find_program(CMAKE_CXX_COMPILER NAMES x86_64-linux-gnu-g++ x86_64-unknown-linux-gnu-g++)
endif()
# FIXME: This could break cross compilation, when the strip is not for the target architecture
find_program(CMAKE_STRIP NAMES x86_64-linux-gnu-strip x86_64-unknown-linux-gnu-strip strip)

if(CMAKE_COMPILER_IS_GNUCC AND CMAKE_COMPILER_IS_GNUCXX)
  set(FLAGS_COMMON_ARCH -ffixed-rbp)
endif()
