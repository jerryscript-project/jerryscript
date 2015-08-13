# Copyright 2015 Samsung Electronics Co., Ltd.
# Copyright 2015 University of Szeged.
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
project (jerry_fdlibm C)

# Compiler / linker flags
set(COMPILE_FLAGS_FDLIBM "${COMPILE_FLAGS_JERRY} ${C_FLAGS_JERRY}")
set(COMPILE_FLAGS_FDLIBM "${COMPILE_FLAGS_FDLIBM} -Wno-error=parentheses")
set(COMPILE_FLAGS_FDLIBM "${COMPILE_FLAGS_FDLIBM} -Wno-error=sign-compare")
set(COMPILE_FLAGS_FDLIBM "${COMPILE_FLAGS_FDLIBM} -Wno-error=sign-conversion")
set(COMPILE_FLAGS_FDLIBM "${COMPILE_FLAGS_FDLIBM} -Wno-error=strict-aliasing")
set(COMPILE_FLAGS_FDLIBM "${COMPILE_FLAGS_FDLIBM} -Wno-error=unknown-pragmas")
set(COMPILE_FLAGS_FDLIBM "${COMPILE_FLAGS_FDLIBM} -Wno-error=missing-declarations")
set(COMPILE_FLAGS_FDLIBM "${COMPILE_FLAGS_FDLIBM} -Wno-error=maybe-uninitialized")
set(COMPILE_FLAGS_FDLIBM "${COMPILE_FLAGS_FDLIBM} -Wno-error=unused-but-set-variable")
set(COMPILE_FLAGS_FDLIBM "${COMPILE_FLAGS_FDLIBM} -Wno-error=unused-variable")
set(COMPILE_FLAGS_FDLIBM "${COMPILE_FLAGS_FDLIBM} -Wno-error=conversion")
set(COMPILE_FLAGS_FDLIBM "${COMPILE_FLAGS_FDLIBM} -Wno-sign-conversion")
set(COMPILE_FLAGS_FDLIBM "${COMPILE_FLAGS_FDLIBM} -Wno-sign-compare")
set(COMPILE_FLAGS_FDLIBM "${COMPILE_FLAGS_FDLIBM} -Wno-parentheses")
set(COMPILE_FLAGS_FDLIBM "${COMPILE_FLAGS_FDLIBM} -Wno-maybe-uninitialized")
set(COMPILE_FLAGS_FDLIBM "${COMPILE_FLAGS_FDLIBM} -Wno-unknown-pragmas")
set(COMPILE_FLAGS_FDLIBM "${COMPILE_FLAGS_FDLIBM} -Wno-unused-but-set-variable")
set(COMPILE_FLAGS_FDLIBM "${COMPILE_FLAGS_FDLIBM} -Wno-unused-variable")

# Include directories
set(INCLUDE_FDLIBM ${CMAKE_SOURCE_DIR}/third-party/fdlibm/include)
set(INCLUDE_FDLIBM ${INCLUDE_FDLIBM} PARENT_SCOPE)

# Source directories
file(GLOB SOURCE_FDLIBM *.c)

add_custom_target (jerry-fdlibm-all)

# Targets declaration
 function(declare_targets_for_build_mode BUILD_MODE)
  set(TARGET_NAME ${BUILD_MODE_PREFIX_${BUILD_MODE}})

  function(declare_target_with_modifiers ) # modifiers are passed in ARGN implicit argument
   foreach(MODIFIER ${ARGN})
    set(TARGET_NAME ${TARGET_NAME}${MODIFIER_SUFFIX_${MODIFIER}})
   endforeach()

   add_library(${TARGET_NAME}.jerry-fdlibm${SUFFIX_THIRD_PARTY_LIB} STATIC ${SOURCE_FDLIBM})
   set_property(TARGET ${TARGET_NAME}.jerry-fdlibm${SUFFIX_THIRD_PARTY_LIB}
                PROPERTY COMPILE_FLAGS "${COMPILE_FLAGS_FDLIBM}")
   target_include_directories(${TARGET_NAME}.jerry-fdlibm${SUFFIX_THIRD_PARTY_LIB} PRIVATE ${INCLUDE_FDLIBM})

   if("${BUILD_MODE}" STREQUAL "UNITTESTS")
    target_include_directories(${TARGET_NAME}.jerry-fdlibm${SUFFIX_THIRD_PARTY_LIB} INTERFACE ${INCLUDE_FDLIBM})
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