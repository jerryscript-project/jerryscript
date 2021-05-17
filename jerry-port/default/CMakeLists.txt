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
set(JERRY_PORT_DEFAULT_NAME jerry-port-default)
project (${JERRY_PORT_DEFAULT_NAME} C)

# Include directories
set(INCLUDE_PORT_DEFAULT "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Source directories
set(SOURCE_PORT_DEFAULT
  default-date.c
  default-debugger.c
  default-external-context.c
  default-fatal.c
  default-io.c
  default-module.c
)

# Amalgamated JerryScript source/header build.
#  The process will create the following files:
#   * jerryscript-port-default.c
#   * jerryscript-port-default.h
if(ENABLE_AMALGAM)
  set(HEADER_PORT_DEFAULT
    include/jerryscript-port-default.h
  )
  set(AMALGAM_PORT_C "${CMAKE_BINARY_DIR}/amalgam/jerryscript-port-default.c")
  set(AMALGAM_PORT_H "${CMAKE_BINARY_DIR}/amalgam/jerryscript-port-default.h")

  add_custom_command(OUTPUT ${AMALGAM_PORT_C} ${AMALGAM_PORT_H}
                     COMMAND ${PYTHON} ${CMAKE_SOURCE_DIR}/tools/amalgam.py
                             --jerry-port-default
                             --output-dir ${CMAKE_BINARY_DIR}/amalgam
                     DEPENDS ${SOURCE_PORT_DEFAULT}
                             ${HEADER_PORT_DEFAULT}
                             ${CMAKE_SOURCE_DIR}/tools/amalgam.py
  )
  add_custom_target(amalgam-port DEPENDS ${AMALGAM_PORT_C} ${AMALGAM_PORT_H})
  add_dependencies(amalgam amalgam-port)

  set(SOURCE_PORT_DEFAULT ${AMALGAM_PORT_C} ${AMALGAM_PORT_H})
endif()

# Define _BSD_SOURCE and _DEFAULT_SOURCE
# (should only be necessary if we used compiler default libc but not checking that)
set(DEFINES_PORT_DEFAULT _BSD_SOURCE _DEFAULT_SOURCE)

INCLUDE (CheckStructHasMember)
# CHECK_STRUCT_HAS_MEMBER works by trying to compile some C code that accesses the
# given field of the given struct. However, our default compiler options break this
# C code, so turn a couple of them off for this.
if(USING_GCC OR USING_CLANG)
  set(CMAKE_REQUIRED_FLAGS "-Wno-error=strict-prototypes -Wno-error=old-style-definition -Wno-error=unused-value")
endif()
# tm.tm_gmtoff is non-standard, so glibc doesn't expose it in c99 mode
# (our default). Define some macros to expose it anyway.
set(CMAKE_REQUIRED_DEFINITIONS "-D_BSD_SOURCE -D_DEFAULT_SOURCE")
CHECK_STRUCT_HAS_MEMBER ("struct tm" tm_gmtoff time.h HAVE_TM_GMTOFF)
# localtime_r is is not threadsafe with clang on OSX
if(HAVE_TM_GMTOFF AND NOT "${PLATFORM}" STREQUAL "DARWIN" AND NOT USING_CLANG)
  set(DEFINES_PORT_DEFAULT ${DEFINES_PORT_DEFAULT} HAVE_TM_GMTOFF)
endif()

# Default Jerry port implementation library
add_library(${JERRY_PORT_DEFAULT_NAME} ${SOURCE_PORT_DEFAULT})
target_include_directories(${JERRY_PORT_DEFAULT_NAME} PUBLIC ${INCLUDE_PORT_DEFAULT})
target_include_directories(${JERRY_PORT_DEFAULT_NAME} PRIVATE ${INCLUDE_CORE_PUBLIC})
target_include_directories(${JERRY_PORT_DEFAULT_NAME} PRIVATE ${INCLUDE_EXT_PUBLIC})
target_compile_definitions(${JERRY_PORT_DEFAULT_NAME} PRIVATE ${DEFINES_PORT_DEFAULT})
target_link_libraries(${JERRY_PORT_DEFAULT_NAME} jerry-core) # FIXME: remove this dependency as soon as possible

# Installation
configure_file(libjerry-port-default.pc.in libjerry-port-default.pc @ONLY)

install(TARGETS ${JERRY_PORT_DEFAULT_NAME} DESTINATION lib)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libjerry-port-default.pc DESTINATION lib/pkgconfig)
install(DIRECTORY ${INCLUDE_PORT_DEFAULT}/ DESTINATION include)
