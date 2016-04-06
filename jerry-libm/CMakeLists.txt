# Copyright 2015-2016 Samsung Electronics Co., Ltd.
# Copyright 2015-2016 University of Szeged.
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
project (Jerry_LibM C)

# Compiler / linker flags
# TODO: Reduce the below list of warning/error disablings as much as possible
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_JERRY} ${C_FLAGS_JERRY}")
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_LIBM} -Wno-error=parentheses")
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_LIBM} -Wno-error=sign-compare")
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_LIBM} -Wno-error=sign-conversion")
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_LIBM} -Wno-error=strict-aliasing")
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_LIBM} -Wno-error=unknown-pragmas")
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_LIBM} -Wno-error=missing-declarations")
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_LIBM} -Wno-error=maybe-uninitialized")
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_LIBM} -Wno-error=unused-but-set-variable")
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_LIBM} -Wno-error=unused-variable")
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_LIBM} -Wno-error=conversion")
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_LIBM} -Wno-sign-conversion")
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_LIBM} -Wno-sign-compare")
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_LIBM} -Wno-parentheses")
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_LIBM} -Wno-maybe-uninitialized")
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_LIBM} -Wno-unknown-pragmas")
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_LIBM} -Wno-unused-but-set-variable")
set(COMPILE_FLAGS_LIBM "${COMPILE_FLAGS_LIBM} -Wno-unused-variable")

# Include directories
set(INCLUDE_LIBM ${CMAKE_SOURCE_DIR}/jerry-libm/include)
set(INCLUDE_LIBM ${INCLUDE_LIBM} PARENT_SCOPE)

# Source directories
file(GLOB SOURCE_LIBM *.c)

add_custom_target (jerry-libm-all)

# Targets declaration
 function(declare_targets_for_build_mode BUILD_MODE)
  set(TARGET_NAME ${BUILD_MODE_PREFIX_${BUILD_MODE}})

  function(declare_target_with_modifiers ) # modifiers are passed in ARGN implicit argument
   foreach(MODIFIER ${ARGN})
    set(TARGET_NAME ${TARGET_NAME}${MODIFIER_SUFFIX_${MODIFIER}})
   endforeach()

   add_library(${TARGET_NAME}.jerry-libm.lib STATIC ${SOURCE_LIBM})
   set_property(TARGET ${TARGET_NAME}.jerry-libm.lib
                PROPERTY COMPILE_FLAGS "${COMPILE_FLAGS_LIBM}")
   target_include_directories(${TARGET_NAME}.jerry-libm.lib PRIVATE ${INCLUDE_LIBM})

   if("${BUILD_MODE}" STREQUAL "UNITTESTS")
    target_include_directories(${TARGET_NAME}.jerry-libm.lib INTERFACE ${INCLUDE_LIBM})
   endif()
  endfunction()

  foreach(MODIFIERS_LIST ${MODIFIERS_LISTS})
   separate_arguments(MODIFIERS_LIST)

   declare_target_with_modifiers(${MODIFIERS_LIST})
  endforeach()
 endfunction()

 declare_targets_for_build_mode(DEBUG)
 declare_targets_for_build_mode(RELEASE)
 declare_targets_for_build_mode(UNITTESTS)
