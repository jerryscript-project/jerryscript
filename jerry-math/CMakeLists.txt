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
set(JERRY_MATH_NAME jerry-math)
project (${JERRY_MATH_NAME} C)

# Compiler / linker flags
# TODO: Reduce the below list of warning/error disablings as much as possible
set(COMPILE_FLAGS_MATH "${COMPILE_FLAGS_MATH} -Wno-error=sign-compare")
set(COMPILE_FLAGS_MATH "${COMPILE_FLAGS_MATH} -Wno-error=sign-conversion")
set(COMPILE_FLAGS_MATH "${COMPILE_FLAGS_MATH} -Wno-sign-conversion")
set(COMPILE_FLAGS_MATH "${COMPILE_FLAGS_MATH} -Wno-sign-compare")
set(COMPILE_FLAGS_MATH "${COMPILE_FLAGS_MATH} -Wno-strict-aliasing")

# Include directories
set(INCLUDE_MATH "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Source directories
file(GLOB SOURCE_MATH *.c)

# "Single" JerryScript libm source/header build.
#  The process will create the following files:
#   * jerryscript-math.c
#   * math.h
if(ENABLE_ALL_IN_ONE_SOURCE)
  file(GLOB HEADER_MATH *.h)
  set(ALL_IN_FILE "${CMAKE_BINARY_DIR}/src/jerryscript-math.c")
  set(ALL_IN_FILE_H "${CMAKE_BINARY_DIR}/src/math.h")

  add_custom_command(OUTPUT ${ALL_IN_FILE} ${ALL_IN_FILE_H} ${JERRYSCRIPT_CONFIG_H}
                     COMMAND python ${CMAKE_SOURCE_DIR}/tools/srcgenerator.py
                             --jerry-math
                             --output-dir ${CMAKE_BINARY_DIR}/src
                     DEPENDS ${SOURCE_MATH}
                             ${HEADER_MATH}
                             ${CMAKE_SOURCE_DIR}/tools/srcgenerator.py
                             ${CMAKE_SOURCE_DIR}/tools/srcmerger.py
  )
  add_custom_target(generate-single-source-math DEPENDS ${ALL_IN_FILE} ${ALL_IN_FILE_H})
  add_dependencies(generate-single-source generate-single-source-math)

  set(SOURCE_MATH ${ALL_IN_FILE} ${ALL_IN_FILE_H})
endif()

add_library(${JERRY_MATH_NAME} ${SOURCE_MATH})
set_property(TARGET ${JERRY_MATH_NAME}
             PROPERTY COMPILE_FLAGS "${COMPILE_FLAGS_MATH}")

target_include_directories(${JERRY_MATH_NAME} PUBLIC ${INCLUDE_MATH})

configure_file(libjerry-math.pc.in libjerry-math.pc @ONLY)

install(TARGETS ${JERRY_MATH_NAME} DESTINATION lib)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libjerry-math.pc DESTINATION lib/pkgconfig)
install(DIRECTORY ${INCLUDE_MATH}/ DESTINATION include/jerryscript-math)
