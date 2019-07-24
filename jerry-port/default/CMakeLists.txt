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
file(GLOB SOURCE_PORT_DEFAULT *.c)

# "Single" JerryScript source/header build.
#  The process will create the following files:
#   * jerryscript-port-default.c
#   * jerryscript-port-default.h
if(ENABLE_ALL_IN_ONE_SOURCE)
  file(GLOB HEADER_PORT_DEFAULT *.h)
  set(ALL_IN_FILE "${CMAKE_BINARY_DIR}/src/jerryscript-port-default.c")
  set(ALL_IN_FILE_H "${CMAKE_BINARY_DIR}/src/jerryscript-port-default.h")

  add_custom_command(OUTPUT ${ALL_IN_FILE} ${ALL_IN_FILE_H} ${JERRYSCRIPT_CONFIG_H}
                     COMMAND python ${CMAKE_SOURCE_DIR}/tools/srcgenerator.py
                             --jerry-port-default
                             --output-dir ${CMAKE_BINARY_DIR}/src
                     DEPENDS ${SOURCE_PORT_DEFAULT}
                             ${HEADER_PORT_DEFAULT}
                             ${CMAKE_SOURCE_DIR}/tools/srcgenerator.py
                             ${CMAKE_SOURCE_DIR}/tools/srcmerger.py
  )
  add_custom_target(generate-single-source-port DEPENDS ${ALL_IN_FILE} ${ALL_IN_FILE_H})
  add_dependencies(generate-single-source generate-single-source-port)

  set(SOURCE_PORT_DEFAULT ${ALL_IN_FILE} ${ALL_IN_FILE_H})
endif()

# Define _BSD_SOURCE and _DEFAULT_SOURCE
# (should only be necessary if we used compiler default libc but not checking that)
set(DEFINES_PORT_DEFAULT _BSD_SOURCE _DEFAULT_SOURCE)

INCLUDE (CheckStructHasMember)
# CHECK_STRUCT_HAS_MEMBER works by trying to compile some C code that accesses the
# given field of the given struct. However, our default compiler options break this
# C code, so turn a couple of them off for this.
set(CMAKE_REQUIRED_FLAGS "-Wno-error=strict-prototypes -Wno-error=old-style-definition")
# tm.tm_gmtoff is non-standard, so glibc doesn't expose it in c99 mode
# (our default). Define some macros to expose it anyway.
set(CMAKE_REQUIRED_DEFINITIONS "-D_BSD_SOURCE -D_DEFAULT_SOURCE")
CHECK_STRUCT_HAS_MEMBER ("struct tm" tm_gmtoff time.h HAVE_TM_GMTOFF)
if(HAVE_TM_GMTOFF)
  set(DEFINES_PORT_DEFAULT ${DEFINES_PORT_DEFAULT} HAVE_TM_GMTOFF)
endif()

# Sleep function availability check
INCLUDE (CheckIncludeFiles)
CHECK_INCLUDE_FILES (time.h HAVE_TIME_H)
CHECK_INCLUDE_FILES (unistd.h HAVE_UNISTD_H)
if(HAVE_TIME_H)
  set(DEFINES_PORT_DEFAULT ${DEFINES_PORT_DEFAULT} HAVE_TIME_H)
elseif(HAVE_UNISTD_H)
  set(DEFINES_PORT_DEFAULT ${DEFINES_PORT_DEFAULT} HAVE_UNISTD_H)
endif()

# Default Jerry port implementation library variants:
#   - default
#   - default-minimal (no extra termination and log APIs)
foreach(JERRY_PORT_LIBRARY_NAME ${JERRY_PORT_DEFAULT_NAME} ${JERRY_PORT_DEFAULT_NAME}-minimal)
  add_library(${JERRY_PORT_LIBRARY_NAME} ${SOURCE_PORT_DEFAULT})
  target_include_directories(${JERRY_PORT_LIBRARY_NAME} PUBLIC ${INCLUDE_PORT_DEFAULT})
  target_include_directories(${JERRY_PORT_LIBRARY_NAME} PRIVATE ${INCLUDE_CORE_PUBLIC})
  target_include_directories(${JERRY_PORT_LIBRARY_NAME} PRIVATE ${INCLUDE_EXT_PUBLIC})
  target_compile_definitions(${JERRY_PORT_LIBRARY_NAME} PRIVATE ${DEFINES_PORT_DEFAULT})
  target_link_libraries(${JERRY_PORT_LIBRARY_NAME} jerry-core) # FIXME: remove this dependency as soon as possible
endforeach()

target_compile_definitions(${JERRY_PORT_DEFAULT_NAME}-minimal PRIVATE DISABLE_EXTRA_API)

# Installation
configure_file(libjerry-port-default.pc.in libjerry-port-default.pc @ONLY)
configure_file(libjerry-port-default-minimal.pc.in libjerry-port-default-minimal.pc @ONLY)

install(TARGETS ${JERRY_PORT_DEFAULT_NAME} ${JERRY_PORT_DEFAULT_NAME}-minimal DESTINATION lib)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libjerry-port-default.pc ${CMAKE_CURRENT_BINARY_DIR}/libjerry-port-default-minimal.pc DESTINATION lib/pkgconfig)
install(DIRECTORY ${INCLUDE_PORT_DEFAULT}/ DESTINATION include)
