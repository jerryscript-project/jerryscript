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
set(JERRY_LIBM_NAME jerry-libm)
project (${JERRY_LIBM_NAME} C)

# Compiler / linker flags
# TODO: Reduce the below list of warning/error disablings as much as possible
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_LIBM} -Wno-error=sign-compare")
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_LIBM} -Wno-error=sign-conversion")
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_LIBM} -Wno-sign-conversion")
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_LIBM} -Wno-sign-compare")
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_LIBM} -Wno-strict-aliasing")

# Include directories
set(INCLUDE_LIBM "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Source directories
file(GLOB SOURCE_LIBM *.c)

# "Single" JerryScript libm source/header build.
#  The process will create the following files:
#   * jerryscript-libm.c
#   * math.h
if(ENABLE_ALL_IN_ONE_SOURCE)
  file(GLOB HEADER_LIBM *.h)
  set(ALL_IN_FILE "${CMAKE_BINARY_DIR}/src/jerryscript-libm.c")
  set(ALL_IN_FILE_H "${CMAKE_BINARY_DIR}/src/math.h")

  add_custom_command(OUTPUT ${ALL_IN_FILE} ${ALL_IN_FILE_H} ${JERRYSCRIPT_CONFIG_H}
                     COMMAND python ${CMAKE_SOURCE_DIR}/tools/srcgenerator.py
                             --jerry-libm
                             --output-dir ${CMAKE_BINARY_DIR}/src
                     DEPENDS ${SOURCE_LIBM}
                             ${HEADER_LIBM}
                             ${CMAKE_SOURCE_DIR}/tools/srcgenerator.py
                             ${CMAKE_SOURCE_DIR}/tools/srcmerger.py
  )
  add_custom_target(generate-single-source-libm DEPENDS ${ALL_IN_FILE} ${ALL_IN_FILE_H})
  add_dependencies(generate-single-source generate-single-source-libm)

  set(SOURCE_LIBM ${ALL_IN_FILE} ${ALL_IN_FILE_H})
endif()

add_library(${JERRY_LIBM_NAME} ${SOURCE_LIBM})
set_property(TARGET ${JERRY_LIBM_NAME}
             PROPERTY COMPILE_FLAGS "${COMPILE_FLAGS_LIBM}")

target_include_directories(${JERRY_LIBM_NAME} PUBLIC ${INCLUDE_LIBM})

configure_file(libjerry-libm.pc.in libjerry-libm.pc @ONLY)

install(TARGETS ${JERRY_LIBM_NAME} DESTINATION lib)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libjerry-libm.pc DESTINATION lib/pkgconfig)
install(DIRECTORY ${INCLUDE_LIBM}/ DESTINATION include/jerry-libm)
