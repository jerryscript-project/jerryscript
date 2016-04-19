# Copyright 2015-2016 Samsung Electronics Co., Ltd.
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

# application name
set(MBEDMODULE "jerry")

# add include jerry-core
set(LJCORE ${CMAKE_CURRENT_LIST_DIR}/../../../)
include_directories(${LJCORE})

# compile flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mlittle-endian -mthumb -mcpu=cortex-m4" )

# link jerryscript
set(LJPATH ${CMAKE_CURRENT_LIST_DIR}/../libjerry)
set(LJFILES "")
set(LJFILES ${LJFILES} ${LJPATH}/libjerrylibm.a)
set(LJFILES ${LJFILES} ${LJPATH}/libjerrycore.a)
target_link_libraries(${MBEDMODULE} ${LJFILES})
