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

cmake_minimum_required (VERSION 2.8.12)
project (Jerry_LibC C ASM)

# Compiler / linker flags
set(COMPILE_FLAGS_LIBC "${COMPILE_FLAGS_JERRY} ${C_FLAGS_JERRY}")

# Definitions
  set(DEFINES_LIBC )

  # Debug
   set(DEFINES_LIBC_DEBUG )

  # Release
   set(DEFINES_LIBC_RELEASE LIBC_NDEBUG)

# Architecture-specific
  # x86_64
   set(DEFINES_LIBC_X86_64 __TARGET_HOST_x64)
  # ARMv7-hf
   set(DEFINES_LIBC_ARMV7_HF __TARGET_HOST_ARMv7 __TARGET_HOST_ARMv7_HARD_FLOAT)
  # ARMv7-el
   set(DEFINES_LIBC_ARMV7_EL __TARGET_HOST_ARMv7 __TARGET_HOST_ARMv7_SOFT_FLOAT)
  # x86
   set(DEFINES_LIBC_X86 __TARGET_HOST_x86)

 # Platform-specific
  # Linux
   set(DEFINES_LIBC_LINUX __TARGET_HOST)
  # Darwin
   set(DEFINES_LIBC_DARWIN __TARGET_HOST)
  # MCU
   set(DEFINES_LIBC_MCU __TARGET_MCU)
   # stm32f3
    set(DEFINES_LIBC_MCU_STM32F3 __TARGET_MCU_STM32F3)
   # stm32f4
    set(DEFINES_LIBC_MCU_STM32F4 __TARGET_MCU_STM32F4)

# Include directories
 set(INCLUDE_LIBC ${CMAKE_SOURCE_DIR}/jerry-libc)
 set(INCLUDE_LIBC_INTERFACE ${CMAKE_SOURCE_DIR}/jerry-libc/include)
 set(INCLUDE_LIBC_INTERFACE ${INCLUDE_LIBC_INTERFACE} PARENT_SCOPE)

 # Third-party
  # Platform-specific
   # Linux
     set(INCLUDE_THIRD_PARTY_LINUX )

   # Darwin
     set(INCLUDE_THIRD_PARTY_DARWIN )

   # MCU
    # STM32F3
     set(INCLUDE_THIRD_PARTY_MCU_STM32F3
         ${CMAKE_SOURCE_DIR}/third-party/STM32F3-Discovery_FW_V1.1.0/Libraries/CMSIS/Device/ST/STM32F30x/Include
         ${CMAKE_SOURCE_DIR}/third-party/STM32F3-Discovery_FW_V1.1.0/Libraries/CMSIS/Include)
    # STM32F4
     set(INCLUDE_THIRD_PARTY_MCU_STM32F4
         ${CMAKE_SOURCE_DIR}/third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/CMSIS/ST/STM32F4xx/Include
         ${CMAKE_SOURCE_DIR}/third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/CMSIS/Include)

# Sources
 file(GLOB SOURCE_LIBC *.c)

 # Platform-specific
  # Linux
   file(GLOB SOURCE_LIBC_LINUX target/linux/*.c target/linux/*.S)

  # Darwin
   file(GLOB SOURCE_LIBC_DARWIN target/darwin/*.c target/darwin/*.S)

  # MCU
   # stm32f3
    file(GLOB SOURCE_LIBC_MCU_STM32F3 target/mcu-stubs/*.c target/mcu-stubs/*.S)
   # stm32f4
    file(GLOB SOURCE_LIBC_MCU_STM32F4 target/mcu-stubs/*.c target/mcu-stubs/*.S)

 # Third-party
  # Platform-specific
   # MCU
    # stm32f3
     set(SOURCE_THIRD_PARTY_MCU_STM32F3
         ${CMAKE_SOURCE_DIR}/third-party/STM32F3-Discovery_FW_V1.1.0/Libraries/CMSIS/Device/ST/STM32F30x/Source/Templates/system_stm32f30x.c
         ${CMAKE_SOURCE_DIR}/third-party/STM32F3-Discovery_FW_V1.1.0/Libraries/CMSIS/Device/ST/STM32F30x/Source/Templates/gcc_ride7/startup_stm32f30x.s)
    # stm32f4
     set(SOURCE_THIRD_PARTY_MCU_STM32F4
         ${CMAKE_SOURCE_DIR}/third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/CMSIS/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c
         ${CMAKE_SOURCE_DIR}/third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/CMSIS/ST/STM32F4xx/Source/Templates/gcc_ride7/startup_stm32f4xx.s)

# Architecture-specific configuration
 if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
  set(DEFINES_LIBC ${DEFINES_LIBC} ${DEFINES_LIBC_X86_64})
 elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "armv7l-hf")
  set(DEFINES_LIBC ${DEFINES_LIBC} ${DEFINES_LIBC_ARMV7_HF})
 elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "armv7l-el")
  set(DEFINES_LIBC ${DEFINES_LIBC} ${DEFINES_LIBC_ARMV7_EL})
 elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "i686")
  set(DEFINES_LIBC ${DEFINES_LIBC} ${DEFINES_LIBC_X86})
 else()
  message(FATAL_ERROR "Unsupported machine architecture")
 endif()

# Platform-specific configuration
 set(DEFINES_LIBC ${DEFINES_LIBC} ${DEFINES_LIBC_${PLATFORM}})

# Targets declaration
 add_custom_target (jerry-libc-all)

 string(TOLOWER ${PLATFORM_EXT} PLATFORM_L)
 set(TARGET_NAME jerry-libc.${PLATFORM_L})

 function(declare_targets_for_build_mode BUILD_MODE)
  set(TARGET_NAME ${BUILD_MODE_PREFIX_${BUILD_MODE}}.${TARGET_NAME})
  set(DEFINES_LIBC ${DEFINES_LIBC} ${DEFINES_LIBC_${BUILD_MODE}})
  set(INCLUDE_LIBC ${INCLUDE_LIBC} ${INCLUDE_LIBC_${PLATFORM_EXT}})

  # Jerry
   add_library(${TARGET_NAME}.lib STATIC ${SOURCE_LIBC} ${SOURCE_LIBC_${PLATFORM_EXT}})
   set_property(TARGET ${TARGET_NAME}.lib
                PROPERTY COMPILE_FLAGS "${COMPILE_FLAGS_LIBC} ${FLAGS_COMMON_${BUILD_MODE}}")
   target_compile_definitions(${TARGET_NAME}.lib PRIVATE ${DEFINES_LIBC})
   target_include_directories(${TARGET_NAME}.lib PRIVATE ${INCLUDE_LIBC})
   target_include_directories(${TARGET_NAME}.lib SYSTEM PRIVATE ${INCLUDE_LIBC_INTERFACE})
   add_dependencies(jerry-libc-all ${TARGET_NAME}.lib)

  # Third-party MCU library
   if(DEFINED SOURCE_THIRD_PARTY_${PLATFORM_EXT})
    add_library(${TARGET_NAME}${SUFFIX_THIRD_PARTY_LIB} STATIC ${SOURCE_THIRD_PARTY_${PLATFORM_EXT}})
    set_property(TARGET ${TARGET_NAME}${SUFFIX_THIRD_PARTY_LIB}
                 PROPERTY COMPILE_FLAGS "${FLAGS_COMMON_${BUILD_MODE}}")
    target_include_directories(${TARGET_NAME}${SUFFIX_THIRD_PARTY_LIB} PRIVATE ${INCLUDE_THIRD_PARTY_${PLATFORM_EXT}})
    target_include_directories(${TARGET_NAME}${SUFFIX_THIRD_PARTY_LIB} SYSTEM PRIVATE ${INCLUDE_LIBC_INTERFACE})
    target_link_libraries(${TARGET_NAME}.lib ${TARGET_NAME}${SUFFIX_THIRD_PARTY_LIB})

    target_link_libraries(${TARGET_NAME}.lib ${TARGET_NAME}${SUFFIX_THIRD_PARTY_LIB})
   endif()
 endfunction()

 declare_targets_for_build_mode(DEBUG)
 declare_targets_for_build_mode(RELEASE)
 declare_targets_for_build_mode(UNITTESTS)
