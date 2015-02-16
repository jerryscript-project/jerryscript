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
project (Jerry CXX C ASM)

# Require g++ of version >= 4.8.0
 if(NOT CMAKE_COMPILER_IS_GNUCXX)
  message(FATAL_ERROR "g++ compiler is required")
 else()
  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
                  OUTPUT_VARIABLE GNU_CXX_VERSION
                  OUTPUT_STRIP_TRAILING_WHITESPACE)

  if(${GNU_CXX_VERSION} VERSION_LESS 4.8.0)
   message(FATAL_ERROR "g++ compiler version 4.8.0 or higher required")
  endif()
 endif()

# Use gcc-ar and gcc-ranlib to support LTO
 get_filename_component(PATH_TO_GCC ${CMAKE_C_COMPILER} REALPATH)
 get_filename_component(DIRECTORY_GCC ${PATH_TO_GCC} DIRECTORY)
 get_filename_component(FILE_NAME_GCC ${PATH_TO_GCC} NAME)
 string(REPLACE "gcc" "gcc-ar" CMAKE_AR ${FILE_NAME_GCC})
 string(REPLACE "gcc" "gcc-ranlib" CMAKE_RANLIB ${FILE_NAME_GCC})
 set(CMAKE_AR ${DIRECTORY_GCC}/${CMAKE_AR})
 set(CMAKE_RANLIB ${DIRECTORY_GCC}/${CMAKE_RANLIB})

# Disable _FORTIFY_SOURCE
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -U_FORTIFY_SOURCE")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -U_FORTIFY_SOURCE")

# Architecture-specific compile/link flags
 foreach(FLAG ${FLAGS_COMMON_ARCH})
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FLAG}")
 endforeach()

# Remove rdynamic option
 set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS )
 set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS )

# Determining platform and defining options
 option(ENABLE_VALGRIND "Enable valgrind helpers in memory allocators" OFF)

 set(PLATFORM "${CMAKE_SYSTEM_NAME}")
 string(TOUPPER "${PLATFORM}" PLATFORM)

 if("${PLATFORM}" STREQUAL "LINUX")
  set(PLATFORM_EXT "LINUX")

  option(STRIP_RELEASE_BINARY "Strip symbols from release binaries" ON)
 elseif("${PLATFORM}" STREQUAL "MCU")
  set(PLATFORM_EXT "MCU_${CMAKE_SYSTEM_VERSION}")

  option(STRIP_RELEASE_BINARY "Strip symbols from release binaries" OFF)
  set(MCU_SCRIPT_FILE "tests/blinky.js" CACHE STRING "Script to run on MCU")
 else()
  message(FATAL_ERROR "Platform is not supported")
 endif()

# Intermediate files
 # Script to run on MCU
  set(MCU_SCRIPT_GENERATED_HEADER ${CMAKE_BINARY_DIR}/generated.h)

# Build modes
 # Debug
  set(BUILD_MODE_PREFIX_DEBUG debug)

 # Release
  set(BUILD_MODE_PREFIX_RELEASE release)

 # Unit tests
  set(BUILD_MODE_PREFIX_UNITTESTS unittests)

# Modifiers
 set(MODIFIERS
     COMPACT_PROFILE
     COMPACT_PROFILE_MINIMAL
     FULL_PROFILE
     MEMORY_STATISTICS)

 # Profiles
  # Full profile (default, so - no suffix)
   set(MODIFIER_SUFFIX_FULL_PROFILE "")

  # Compact profile
   set(MODIFIER_SUFFIX_COMPACT_PROFILE -cp)

  # Minimal compact profile
   set(MODIFIER_SUFFIX_COMPACT_PROFILE_MINIMAL -cp_minimal)

 # Memory statistics
  set(MODIFIER_SUFFIX_MEMORY_STATISTICS -mem_stats)

# Modifier lists
 # Linux
  set(MODIFIERS_LISTS_LINUX
     "FULL_PROFILE"
     "COMPACT_PROFILE"
     "COMPACT_PROFILE_MINIMAL"
     "FULL_PROFILE MEMORY_STATISTICS"
     "COMPACT_PROFILE_MINIMAL MEMORY_STATISTICS")

 # MCU
  # stm32f3
   set(MODIFIERS_LISTS_MCU_STM32F3
      "FULL_PROFILE"
      "COMPACT_PROFILE"
      "COMPACT_PROFILE_MINIMAL")

  # stm32f4
   set(MODIFIERS_LISTS_MCU_STM32F4
      "FULL_PROFILE"
      "COMPACT_PROFILE"
      "COMPACT_PROFILE_MINIMAL")

# Compiler / Linker flags
 set(COMPILE_FLAGS_JERRY "-fno-builtin")
 set(LINKER_FLAGS_COMMON "")

 # Turn off stack protector
  set(COMPILE_FLAGS_JERRY "${COMPILE_FLAGS_JERRY} -fno-stack-protector")

 # Debug information
  set(COMPILE_FLAGS_JERRY "${COMPILE_FLAGS_JERRY} -g -gdwarf-4")
 
 # Warnings
  set(COMPILE_FLAGS_JERRY "${COMPILE_FLAGS_JERRY} -Wall -Wextra -Wpedantic -Wlogical-op")
  set(COMPILE_FLAGS_JERRY "${COMPILE_FLAGS_JERRY} -Wformat-nonliteral -Winit-self -Wno-stack-protector")
  set(COMPILE_FLAGS_JERRY "${COMPILE_FLAGS_JERRY} -Wconversion -Wsign-conversion -Wformat-security")
  set(COMPILE_FLAGS_JERRY "${COMPILE_FLAGS_JERRY} -Wmissing-declarations -Wno-attributes")
  set(COMPILE_FLAGS_JERRY "${COMPILE_FLAGS_JERRY} -Werror -Wfatal-errors")

 # Static build
   set(LINKER_FLAGS_STATIC "-static")

 # C++
  set(CXX_FLAGS_JERRY "-std=c++11 -fno-exceptions -fno-rtti")

  # Turn off implicit template instantiation
   set(CXX_FLAGS_JERRY "${CXX_FLAGS_JERRY} -fno-implicit-templates -fno-implicit-inline-templates")

 # C
  set(C_FLAGS_JERRY "-std=c99")

 # Platform-specific
  # MCU
   # stm32f3
    set(LINKER_FLAGS_COMMON_MCU_STM32F3 "-T${CMAKE_SOURCE_DIR}/third-party/stm32f3.ld")
   # stm32f4
    set(LINKER_FLAGS_COMMON_MCU_STM32F4 "-T${CMAKE_SOURCE_DIR}/third-party/stm32f4.ld")

 # Debug
  set(FLAGS_COMMON_DEBUG "-nostdlib")
  set(LINKER_FLAGS_COMMON_DEBUG "-nostdlib")

 # Release
  set(FLAGS_COMMON_RELEASE "-Os -flto -nostdlib")
  set(LINKER_FLAGS_COMMON_RELEASE "-nostdlib")

 # Unit tests
  set(FLAGS_COMMON_UNITTESTS "-O3 -flto -nodefaultlibs")

# Include directories
 # Core interface
  set(INCLUDE_CORE_INTERFACE
      ${CMAKE_SOURCE_DIR}/core)

# Sources
 # Platform-specific
  # Jerry standalone
   # Linux
    set(SOURCE_JERRY_STANDALONE_MAIN_LINUX main-linux.cpp)

   # MCU
    # stm32f3
     set(SOURCE_JERRY_STANDALONE_MAIN_MCU_STM32F3 main-mcu.cpp)

    # stm32f4
     set(SOURCE_JERRY_STANDALONE_MAIN_MCU_STM32F4 main-mcu.cpp)

 # Unit tests main modules
  file(GLOB SOURCE_UNIT_TEST_MAIN_MODULES tests/unit/*.cpp)

# Imported libraries
 # libc
  add_library(imported_libc SHARED IMPORTED)
  execute_process(COMMAND ${CMAKE_C_COMPILER} ${FLAGS_COMMON_ARCH} -print-file-name=libc.so
                  OUTPUT_VARIABLE IMPORTED_LIBC_LOCATION
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  set_property(TARGET imported_libc
               PROPERTY IMPORTED_LOCATION ${IMPORTED_LIBC_LOCATION})
 # libgcc
  add_library(imported_libgcc STATIC IMPORTED)
  execute_process(COMMAND ${CMAKE_C_COMPILER} ${FLAGS_COMMON_ARCH} -print-file-name=libgcc.a
                  OUTPUT_VARIABLE IMPORTED_LIBGCC_LOCATION
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  set_property(TARGET imported_libgcc
               PROPERTY IMPORTED_LOCATION ${IMPORTED_LIBGCC_LOCATION})

# Platform-specific configuration
 set(MODIFIERS_LISTS ${MODIFIERS_LISTS_${PLATFORM_EXT}})
 set(LINKER_FLAGS_COMMON "${LINKER_FLAGS_COMMON} ${LINKER_FLAGS_COMMON_${PLATFORM_EXT}}")
 set(SOURCE_JERRY_STANDALONE_MAIN ${SOURCE_JERRY_STANDALONE_MAIN_${PLATFORM_EXT}})

# Component targets
 # Jerry's Core
  add_subdirectory(core)

 # Jerry's libc
  add_subdirectory(jerry-libc)

 # Plugins
  add_subdirectory(plugins)

# Targets declaration
 function(declare_targets_for_build_mode BUILD_MODE)
  string(TOLOWER "${PLATFORM_EXT}" PLATFORM_L)
  set(TARGET_NAME ${BUILD_MODE_PREFIX_${BUILD_MODE}}.${PLATFORM_L})
  set(LIBC_TARGET_NAME ${BUILD_MODE_PREFIX_${BUILD_MODE}}.jerry-libc.${PLATFORM_L})

  function(declare_target_with_modifiers ) # modifiers are passed in ARGN implicit argument
   set(CORE_TARGET_NAME ${BUILD_MODE_PREFIX_${BUILD_MODE}})
   foreach(MODIFIER ${ARGN}) # FIXME
    set(TARGET_NAME ${TARGET_NAME}${MODIFIER_SUFFIX_${MODIFIER}})

    set(LIBC_TARGET_NAME ${LIBC_TARGET_NAME}${MODIFIER_SUFFIX_${MODIFIER}})
    set(CORE_TARGET_NAME ${CORE_TARGET_NAME}${MODIFIER_SUFFIX_${MODIFIER}})
   endforeach()
   set(CORE_TARGET_NAME ${CORE_TARGET_NAME}.jerry-core)
   set(LIBC_TARGET_NAME ${LIBC_TARGET_NAME}.lib)

   set(DEFINES_JERRY )

   if("${PLATFORM}" STREQUAL "MCU")
    set(MCU_SCRIPT_GENERATED_HEADER ${MCU_SCRIPT_GENERATED_HEADER}.${TARGET_NAME})
    add_custom_command(OUTPUT ${MCU_SCRIPT_GENERATED_HEADER}
                       COMMAND ${CMAKE_SOURCE_DIR}/tools/generator.sh "${CMAKE_SOURCE_DIR}/${MCU_SCRIPT_FILE}" ${MCU_SCRIPT_GENERATED_HEADER}
                       WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    add_custom_target(mcu_header_with_script_to_run.${TARGET_NAME} DEPENDS ${MCU_SCRIPT_GENERATED_HEADER})
    set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_MCU_SCRIPT_HEADER="${MCU_SCRIPT_GENERATED_HEADER}")
   endif()

   add_executable(${TARGET_NAME} ${SOURCE_JERRY_STANDALONE_MAIN})
   set_property(TARGET ${TARGET_NAME}
                PROPERTY COMPILE_FLAGS "${COMPILE_FLAGS_JERRY} ${CXX_FLAGS_JERRY} ${FLAGS_COMMON_${BUILD_MODE}}")
   set_property(TARGET ${TARGET_NAME}
                PROPERTY LINK_FLAGS "${COMPILE_FLAGS_JERRY} ${CXX_FLAGS_JERRY} ${FLAGS_COMMON_${BUILD_MODE}} ${LINKER_FLAGS_COMMON} ${LINKER_FLAGS_STATIC}")
   target_compile_definitions(${TARGET_NAME} PRIVATE ${DEFINES_JERRY})
   target_include_directories(${TARGET_NAME} PRIVATE ${INCLUDE_CORE_INTERFACE})
   target_link_libraries(${TARGET_NAME} ${CORE_TARGET_NAME} ${LIBC_TARGET_NAME} imported_libgcc)

   if("${PLATFORM}" STREQUAL "MCU")
    add_dependencies(${TARGET_NAME} mcu_header_with_script_to_run.${TARGET_NAME})
    add_custom_target(${TARGET_NAME}.bin
                      COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${TARGET_NAME}> $<TARGET_FILE:${TARGET_NAME}>.bin
                      DEPENDS ${TARGET_NAME})
    add_custom_target(${TARGET_NAME}.flash
                      COMMAND st-flash write $<TARGET_FILE:${TARGET_NAME}>.bin 0x08000000
                      DEPENDS ${TARGET_NAME}.bin)
   endif()

   if("${BUILD_MODE}" STREQUAL "RELEASE")
    if(${STRIP_RELEASE_BINARY} STREQUAL "ON")
     add_custom_command(TARGET ${TARGET_NAME}
                        POST_BUILD
                        COMMAND ${CMAKE_STRIP} $<TARGET_FILE:${TARGET_NAME}>)
    endif()
   endif()
  endfunction()

  foreach(MODIFIERS_LIST ${MODIFIERS_LISTS})
   separate_arguments(MODIFIERS_LIST)

   declare_target_with_modifiers(${MODIFIERS_LIST})
  endforeach()
 endfunction()

 declare_targets_for_build_mode(DEBUG)
 declare_targets_for_build_mode(RELEASE)

 # Unit tests declaration
  add_custom_target(unittests)

  foreach(SOURCE_UNIT_TEST_MAIN ${SOURCE_UNIT_TEST_MAIN_MODULES})
   get_filename_component(TARGET_NAME ${SOURCE_UNIT_TEST_MAIN} NAME_WE)
   set(TARGET_NAME unit_${TARGET_NAME})

   set(CORE_TARGET_NAME unittests.jerry-core)

   add_executable(${TARGET_NAME} ${SOURCE_UNIT_TEST_MAIN})
   set_property(TARGET ${TARGET_NAME}
                PROPERTY COMPILE_FLAGS "${COMPILE_FLAGS_JERRY} ${CXX_FLAGS_JERRY} ${FLAGS_UNIT_TEST}")
   set_property(TARGET ${TARGET_NAME}
                PROPERTY LINK_FLAGS "${COMPILE_FLAGS_JERRY} ${CXX_FLAGS_JERRY} ${FLAGS_COMMON_UNITTESTS} ${LINKER_FLAGS_COMMON}")
   target_compile_definitions(${TARGET_NAME} PRIVATE ${DEFINES_UNIT_TEST})
   target_include_directories(${TARGET_NAME} PRIVATE ${INCLUDE_CORE})
   target_link_libraries(${TARGET_NAME} ${CORE_TARGET_NAME} imported_libc imported_libgcc)
 
   add_dependencies(unittests ${TARGET_NAME})
  endforeach()
