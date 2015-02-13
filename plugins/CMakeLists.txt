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
project (Jerry_Plugins CXX ASM)

# Compiler / linker flags
 set(COMPILE_FLAGS_PLUGINS "${COMPILE_FLAGS_JERRY} ${CXX_FLAGS_JERRY}")

# Definitions
  # Debug
   set(DEFINES_PLUGINS_DEBUG )

  # Release
   set(DEFINES_PLUGINS_RELEASE JERRY_NDEBUG)

 # Platform-specific
  # Linux
   set(DEFINES_PLUGINS_LINUX __TARGET_HOST)
  # MCU
   # stm32f3
    set(DEFINES_PLUGINS_MCU_STM32F3 __TARGET_MCU __TARGET_MCU_STM32F3)
   # stm32f4
    set(DEFINES_PLUGINS_MCU_STM32F4 __TARGET_MCU __TARGET_MCU_STM32F4)

# Include directories
 set(INCLUDE_PLUGINS
     ${CMAKE_SOURCE_DIR}/src
     lib-device-stm)

 # Third-party
  # Platform-specific
   # Linux
     set(INCLUDE_THIRD_PARTY_LINUX )

   # MCU
    # STM32F3
     set(INCLUDE_THIRD_PARTY_MCU_STM32F3
         ${CMAKE_SOURCE_DIR}/third-party/STM32F3-Discovery_FW_V1.1.0/Libraries/CMSIS/Device/ST/STM32F30x/Include
         ${CMAKE_SOURCE_DIR}/third-party/STM32F3-Discovery_FW_V1.1.0/Libraries/STM32F30x_StdPeriph_Driver/inc
         ${CMAKE_SOURCE_DIR}/third-party/STM32F3-Discovery_FW_V1.1.0/Libraries/CMSIS/Include
         ${CMAKE_SOURCE_DIR}/third-party/STM32F3-Discovery_FW_V1.1.0)
    # STM32F4
     set(INCLUDE_THIRD_PARTY_MCU_STM32F4
        ${CMAKE_SOURCE_DIR}/third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/CMSIS/ST/STM32F4xx/Include
        ${CMAKE_SOURCE_DIR}/third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/STM32F4xx_StdPeriph_Driver/inc
        ${CMAKE_SOURCE_DIR}/third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/CMSIS/Include
        ${CMAKE_SOURCE_DIR}/third-party/STM32F4-Discovery_FW_V1.1.0)

# Sources
 file(GLOB SOURCE_PLUGINS lib-device-stm/*.cpp)

 # Third-party
  # Platform-specific
   # MCU
    # stm32f3
     set(SOURCE_THIRD_PARTY_MCU_STM32F3
         ${CMAKE_SOURCE_DIR}/third-party/STM32F3-Discovery_FW_V1.1.0/Libraries/CMSIS/Device/ST/STM32F30x/Source/Templates/system_stm32f30x.c
         ${CMAKE_SOURCE_DIR}/third-party/STM32F3-Discovery_FW_V1.1.0/Libraries/STM32F30x_StdPeriph_Driver/src/stm32f30x_tim.c
         ${CMAKE_SOURCE_DIR}/third-party/STM32F3-Discovery_FW_V1.1.0/Libraries/STM32F30x_StdPeriph_Driver/src/stm32f30x_gpio.c
         ${CMAKE_SOURCE_DIR}/third-party/STM32F3-Discovery_FW_V1.1.0/Libraries/STM32F30x_StdPeriph_Driver/src/stm32f30x_rcc.c)
    # stm32f4
     set(SOURCE_THIRD_PARTY_MCU_STM32F4
         ${CMAKE_SOURCE_DIR}/third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/CMSIS/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c
         ${CMAKE_SOURCE_DIR}/third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_tim.c
         ${CMAKE_SOURCE_DIR}/third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_gpio.c
         ${CMAKE_SOURCE_DIR}/third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_rcc.c)

# Platform-specific configuration
 set(DEFINES_PLUGINS ${DEFINES_PLUGINS_${PLATFORM_EXT}})
 set(INCLUDE_PLUGINS ${INCLUDE_PLUGINS} ${INCLUDE_THIRD_PARTY_${PLATFORM_EXT}})

# Targets declaration 
 string(TOLOWER ${PLATFORM_EXT} PLATFORM_L)
 set(TARGET_NAME plugins.${PLATFORM_L})

 function(declare_targets_for_build_mode BUILD_MODE)
  set(TARGET_NAME ${BUILD_MODE_PREFIX_${BUILD_MODE}}.${TARGET_NAME})
  set(DEFINES_PLUGINS ${DEFINES_PLUGINS} ${DEFINES_PLUGINS_${BUILD_MODE}})

  # Jerry
   add_library(${TARGET_NAME}.lib STATIC ${SOURCE_PLUGINS})
   set_property(TARGET ${TARGET_NAME}.lib
                PROPERTY COMPILE_FLAGS "${COMPILE_FLAGS_PLUGINS} ${FLAGS_COMMON_${BUILD_MODE}}")
   target_compile_definitions(${TARGET_NAME}.lib PRIVATE ${DEFINES_PLUGINS})
   target_include_directories(${TARGET_NAME}.lib PRIVATE ${INCLUDE_PLUGINS})

  # Third-party MCU library
   if(DEFINED SOURCE_THIRD_PARTY_${PLATFORM_EXT})
    add_library(${TARGET_NAME}.third_party.lib STATIC ${SOURCE_THIRD_PARTY_${PLATFORM_EXT}})
    set_property(TARGET ${TARGET_NAME}.third_party.lib
                 PROPERTY COMPILE_FLAGS "${FLAGS_COMMON_${BUILD_MODE}}")
    target_include_directories(${TARGET_NAME}.third_party.lib PRIVATE ${INCLUDE_PLUGINS})

    target_link_libraries(${TARGET_NAME}.lib ${TARGET_NAME}.third_party.lib)
   endif()
 endfunction()

 foreach(BUILD_MODE ${BUILD_MODES})
  declare_targets_for_build_mode(${BUILD_MODE})
 endforeach()
