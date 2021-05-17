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
set(COMPILE_FLAGS_MATH_GCC_CLANG "${COMPILE_FLAGS_MATH_GCC_CLANG} -Wno-error=sign-compare")
set(COMPILE_FLAGS_MATH_GCC_CLANG "${COMPILE_FLAGS_MATH_GCC_CLANG} -Wno-error=sign-conversion")
set(COMPILE_FLAGS_MATH_GCC_CLANG "${COMPILE_FLAGS_MATH_GCC_CLANG} -Wno-sign-conversion")
set(COMPILE_FLAGS_MATH_GCC_CLANG "${COMPILE_FLAGS_MATH_GCC_CLANG} -Wno-sign-compare")
set(COMPILE_FLAGS_MATH_GCC_CLANG "${COMPILE_FLAGS_MATH_GCC_CLANG} -Wno-strict-aliasing")

# Include directories
set(INCLUDE_MATH "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Source directories
set(SOURCE_MATH
  acos.c
  acosh.c
  asin.c
  asinh.c
  atan.c
  atan2.c
  atanh.c
  cbrt.c
  ceil.c
  copysign.c
  cosh.c
  exp.c
  expm1.c
  fabs.c
  floor.c
  fmod.c
  log.c
  log10.c
  log1p.c
  log2.c
  nextafter.c
  pow.c
  scalbn.c
  sinh.c
  sqrt.c
  tanh.c
  trig.c
)

# Amalgamated JerryScript source/header build.
#  The process will create the following files:
#   * jerryscript-math.c
#   * math.h
if(ENABLE_AMALGAM)
  set(HEADER_MATH
    include/math.h
    jerry-math-internal.h
  )
  set(AMALGAM_MATH_C "${CMAKE_BINARY_DIR}/amalgam/jerryscript-math.c")
  set(AMALGAM_MATH_H "${CMAKE_BINARY_DIR}/amalgam/math.h")

  add_custom_command(OUTPUT ${AMALGAM_MATH_C} ${AMALGAM_MATH_H}
                     COMMAND ${PYTHON} ${CMAKE_SOURCE_DIR}/tools/amalgam.py
                             --jerry-math
                             --output-dir ${CMAKE_BINARY_DIR}/amalgam
                     DEPENDS ${SOURCE_MATH}
                             ${HEADER_MATH}
                             ${CMAKE_SOURCE_DIR}/tools/amalgam.py
  )
  add_custom_target(amalgam-math DEPENDS ${AMALGAM_MATH_C} ${AMALGAM_MATH_H})
  add_dependencies(amalgam amalgam-math)

  set(SOURCE_MATH ${AMALGAM_MATH_C} ${AMALGAM_MATH_H})
endif()

add_library(${JERRY_MATH_NAME} ${SOURCE_MATH})
if(USING_GCC OR USING_CLANG)
  set_property(TARGET ${JERRY_MATH_NAME}
               PROPERTY COMPILE_FLAGS "${COMPILE_FLAGS_MATH_GCC_CLANG}")
endif()
target_include_directories(${JERRY_MATH_NAME} PUBLIC ${INCLUDE_MATH})

configure_file(libjerry-math.pc.in libjerry-math.pc @ONLY)

install(TARGETS ${JERRY_MATH_NAME} DESTINATION lib)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libjerry-math.pc DESTINATION lib/pkgconfig)
install(DIRECTORY ${INCLUDE_MATH}/ DESTINATION include/jerryscript-math)
