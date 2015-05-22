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

# Require g++ of version >= 4.7.0
 if(NOT CMAKE_COMPILER_IS_GNUCXX)
  message(FATAL_ERROR "g++ compiler is required")
 else()
  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
                  OUTPUT_VARIABLE GNU_CXX_VERSION
                  OUTPUT_STRIP_TRAILING_WHITESPACE)

  if(${GNU_CXX_VERSION} VERSION_LESS 4.7.0)
   message(FATAL_ERROR "g++ compiler version 4.7.0 or higher required")
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

# Imported and third-party targets prefix
 set(PREFIX_IMPORTED_LIB imported_)
 set(SUFFIX_THIRD_PARTY_LIB .third_party.lib)

# Static checkers
 include(build/static-checkers/add_cppcheck_for_target.cmake)

 add_custom_target(static_check)
 add_dependencies(static_check cppcheck)

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
 option(ENABLE_LTO      "Enable LTO build" ON)
 option(ENABLE_LOG      "Enable LOG build" OFF)

 set(PLATFORM "${CMAKE_SYSTEM_NAME}")
 string(TOUPPER "${PLATFORM}" PLATFORM)

 if("${PLATFORM}" STREQUAL "LINUX")
  set(PLATFORM_EXT "LINUX")
  set(EXTERNAL_BUILD FALSE)

  option(STRIP_RELEASE_BINARY "Strip symbols from release binaries" ON)
 elseif("${PLATFORM}" STREQUAL "MCU")
  set(PLATFORM_EXT "MCU_${CMAKE_SYSTEM_VERSION}")
  set(EXTERNAL_BUILD FALSE)

  option(STRIP_RELEASE_BINARY "Strip symbols from release binaries" OFF)
  set(MCU_SCRIPT_FILE "tests/blinky.js" CACHE STRING "Script to run on MCU")
 elseif("${PLATFORM}" STREQUAL "EXTERNAL")
  set(PLATFORM_EXT "${CMAKE_SYSTEM_VERSION}")
  set(EXTERNAL_BUILD TRUE)

  set(EXTERNAL_LIBC_INTERFACE "UNDEFINED" CACHE STRING "Path to external libc include directory")
  set(EXTERNAL_LIBS_INTERFACE "UNDEFINED" CACHE STRING "Path to external libraries' include directory")
 else()
  message(FATAL_ERROR "Platform is not supported")
 endif()

# Intermediate files
 # Script to run on MCU
  set(MCU_SCRIPT_GENERATED_HEADER ${CMAKE_BINARY_DIR}/generated.h)

# Should we use external libc?
 if(NOT DEFINED EXTERNAL_LIBC_INTERFACE OR EXTERNAL_LIBC_INTERFACE STREQUAL "UNDEFINED")
  set(USE_JERRY_LIBC TRUE)

  # Jerry's libc doesn't support Nuttx platform
   if(${PLATFORM_EXT} STREQUAL "NUTTX")
    message(FATAL_ERROR "Nuttx build doesn't support Jerry's libc implementation")
   endif()
 else()
  set(USE_JERRY_LIBC FALSE)

  if(NOT EXISTS "${EXTERNAL_LIBC_INTERFACE}/stdlib.h")
   message(FATAL_ERROR "It seems that external libc interface '${EXTERNAL_LIBC_INTERFACE}' doesn't provide stdlib.h header")
  endif()

  set(INCLUDE_LIBC_INTERFACE ${EXTERNAL_LIBC_INTERFACE})
 endif()

# Are there any interfaces for external libraries, other than libc, that should be registered?
 if(DEFINED EXTERNAL_LIBS_INTERFACE AND NOT EXTERNAL_LIBS_INTERFACE STREQUAL "UNDEFINED")
  set(INCLUDE_EXTERNAL_LIBS_INTERFACE )

  foreach(EXTERNAL_LIB_INTERFACE ${EXTERNAL_LIBS_INTERFACE})
   if (NOT EXISTS "${EXTERNAL_LIB_INTERFACE}")
    message(FATAL_ERROR "Interface directory of the external library doesn't exist: ${EXTERNAL_LIB_INTERFACE}")
   endif()

   set(INCLUDE_EXTERNAL_LIBS_INTERFACE ${INCLUDE_EXTERNAL_LIBS_INTERFACE} ${EXTERNAL_LIB_INTERFACE})
  endforeach()
 endif()

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
     MINIMAL_FOOTPRINT
     MEMORY_STATISTICS)

 # Profiles
  # Full profile (default, so - no suffix)
   set(MODIFIER_SUFFIX_FULL_PROFILE "")

  # Compact profile
   set(MODIFIER_SUFFIX_COMPACT_PROFILE -cp)

  # Minimal compact profile
   set(MODIFIER_SUFFIX_COMPACT_PROFILE_MINIMAL -cp_minimal)

  # Minimal footprint
   set(MODIFIER_SUFFIX_MINIMAL_FOOTPRINT -mfp)

 # Memory statistics
  set(MODIFIER_SUFFIX_MEMORY_STATISTICS -mem_stats)

# Modifier lists
 # Linux
  set(MODIFIERS_LISTS_LINUX
     "FULL_PROFILE"
     "COMPACT_PROFILE"
     "COMPACT_PROFILE_MINIMAL"
     "FULL_PROFILE MINIMAL_FOOTPRINT"
     "FULL_PROFILE MINIMAL_FOOTPRINT MEMORY_STATISTICS"
     "FULL_PROFILE MEMORY_STATISTICS"
     "COMPACT_PROFILE_MINIMAL MINIMAL_FOOTPRINT"
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

 # Nuttx
  set(MODIFIERS_LISTS_NUTTX
      ${MODIFIERS_LISTS_LINUX})

# Compiler / Linker flags
 set(COMPILE_FLAGS_JERRY "-fno-builtin")
 set(LINKER_FLAGS_COMMON "-Wl,-z,noexecstack")

 # LTO
  if("${ENABLE_LTO}" STREQUAL "ON")
   set(COMPILE_FLAGS_JERRY "${COMPILE_FLAGS_JERRY} -flto -fno-fat-lto-objects")
   set(LINKER_FLAGS_COMMON "${LINKER_FLAGS_COMMON} -flto")
  endif()

 # Turn off stack protector
  set(COMPILE_FLAGS_JERRY "${COMPILE_FLAGS_JERRY} -fno-stack-protector")

 # Debug information
  set(COMPILE_FLAGS_JERRY "${COMPILE_FLAGS_JERRY} -g -gdwarf-4")

 # Warnings
  set(COMPILE_FLAGS_JERRY "${COMPILE_FLAGS_JERRY} -Wall -Wextra -pedantic -Wlogical-op")
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
    set(LINKER_FLAGS_COMMON_MCU_STM32F3 "-T${CMAKE_SOURCE_DIR}/third-party/STM32F3-Discovery_FW_V1.1.0/Project/Peripheral_Examples/FLASH_Program/TrueSTUDIO/FLASH_Program/STM32_FLASH.ld")
   # stm32f4
    set(LINKER_FLAGS_COMMON_MCU_STM32F4 "-T${CMAKE_SOURCE_DIR}/third-party/STM32F4-Discovery_FW_V1.1.0/Project/Peripheral_Examples/FLASH_Program/TrueSTUDIO/FLASH_Program/stm32_flash.ld")

 # Debug
  set(FLAGS_COMMON_DEBUG "-nostdlib")

 # Release
  set(FLAGS_COMMON_RELEASE "-Os -nostdlib")

 # Unit tests
  set(FLAGS_COMMON_UNITTESTS "-O3 -nodefaultlibs")

# Include directories
 # Core interface
  set(INCLUDE_CORE_INTERFACE
      ${CMAKE_SOURCE_DIR}/jerry-core)

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

   # Nuttx
    set(SOURCE_JERRY_STANDALONE_MAIN_NUTTX main-nuttx.cpp)

 # Unit tests main modules
  file(GLOB SOURCE_UNIT_TEST_MAIN_MODULES tests/unit/*.cpp)

# Imported libraries
 # libc
  add_library(${PREFIX_IMPORTED_LIB}libc SHARED IMPORTED)
  execute_process(COMMAND ${CMAKE_C_COMPILER} ${FLAGS_COMMON_ARCH} -print-file-name=libc.so
                  OUTPUT_VARIABLE IMPORTED_LIBC_LOCATION
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  set_property(TARGET ${PREFIX_IMPORTED_LIB}libc
               PROPERTY IMPORTED_LOCATION ${IMPORTED_LIBC_LOCATION})
 # libgcc
  add_library(${PREFIX_IMPORTED_LIB}libgcc STATIC IMPORTED)
  execute_process(COMMAND ${CMAKE_C_COMPILER} ${FLAGS_COMMON_ARCH} -print-file-name=libgcc.a
                  OUTPUT_VARIABLE IMPORTED_LIBGCC_LOCATION
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  set_property(TARGET ${PREFIX_IMPORTED_LIB}libgcc
               PROPERTY IMPORTED_LOCATION ${IMPORTED_LIBGCC_LOCATION})
 # libgcc_eh
  add_library(${PREFIX_IMPORTED_LIB}libgcc_eh STATIC IMPORTED)
  execute_process(COMMAND ${CMAKE_C_COMPILER} ${FLAGS_COMMON_ARCH} -print-file-name=libgcc_eh.a
                  OUTPUT_VARIABLE IMPORTED_LIBGCC_EH_LOCATION
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(EXISTS "${IMPORTED_LIBGCC_EH_LOCATION}")
   set_property(TARGET ${PREFIX_IMPORTED_LIB}libgcc_eh
                PROPERTY IMPORTED_LOCATION ${IMPORTED_LIBGCC_EH_LOCATION})
  else()
   # If libgcc_eh not found, reference libgcc instead
   set_property(TARGET ${PREFIX_IMPORTED_LIB}libgcc_eh
                PROPERTY IMPORTED_LOCATION ${IMPORTED_LIBGCC_LOCATION})
  endif()

# Platform-specific configuration
 set(MODIFIERS_LISTS ${MODIFIERS_LISTS_${PLATFORM_EXT}})
 set(LINKER_FLAGS_COMMON "${LINKER_FLAGS_COMMON} ${LINKER_FLAGS_COMMON_${PLATFORM_EXT}}")
 set(SOURCE_JERRY_STANDALONE_MAIN ${SOURCE_JERRY_STANDALONE_MAIN_${PLATFORM_EXT}})

# Component targets
 # Jerry's libc
  if(${USE_JERRY_LIBC})
   add_subdirectory(jerry-libc)
  endif()

 # Jerry's fdlibm
  add_subdirectory(third-party/fdlibm)

 # Jerry's Core
  add_subdirectory(jerry-core)

 # Plugins
  add_subdirectory(plugins)

# Targets declaration
 function(declare_targets_for_build_mode BUILD_MODE)
  string(TOLOWER "${PLATFORM_EXT}" PLATFORM_L)
  set(TARGET_NAME ${BUILD_MODE_PREFIX_${BUILD_MODE}}.${PLATFORM_L})
  set(PLUGINS_TARGET_NAME ${BUILD_MODE_PREFIX_${BUILD_MODE}}.plugins.${PLATFORM_L}.lib)
  set(LIBC_TARGET_NAME ${BUILD_MODE_PREFIX_${BUILD_MODE}}.jerry-libc.${PLATFORM_L}.lib)

  function(declare_target_with_modifiers ) # modifiers are passed in ARGN implicit argument
   set(CORE_TARGET_NAME ${BUILD_MODE_PREFIX_${BUILD_MODE}})
   foreach(MODIFIER ${ARGN})
    set(TARGET_NAME ${TARGET_NAME}${MODIFIER_SUFFIX_${MODIFIER}})

    set(CORE_TARGET_NAME ${CORE_TARGET_NAME}${MODIFIER_SUFFIX_${MODIFIER}})
   endforeach()
   set(FDLIBM_TARGET_NAME ${CORE_TARGET_NAME}.jerry-fdlibm${SUFFIX_THIRD_PARTY_LIB})
   set(CORE_TARGET_NAME ${CORE_TARGET_NAME}.jerry-core)

   set(DEFINES_JERRY )

   if(NOT ${EXTERNAL_BUILD})
    add_executable(${TARGET_NAME} ${SOURCE_JERRY_STANDALONE_MAIN})

    if("${PLATFORM}" STREQUAL "MCU")
     set(MCU_SCRIPT_GENERATED_HEADER ${MCU_SCRIPT_GENERATED_HEADER}.${TARGET_NAME})
     add_custom_command(OUTPUT ${MCU_SCRIPT_GENERATED_HEADER}
                        COMMAND ${CMAKE_SOURCE_DIR}/tools/generator.sh "${CMAKE_SOURCE_DIR}/${MCU_SCRIPT_FILE}" ${MCU_SCRIPT_GENERATED_HEADER}
                        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
     add_custom_target(mcu_header_with_script_to_run.${TARGET_NAME} DEPENDS ${MCU_SCRIPT_GENERATED_HEADER})
     set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_MCU_SCRIPT_HEADER="${MCU_SCRIPT_GENERATED_HEADER}")
    elseif("${PLATFORM}" STREQUAL "LINUX")
     if("${ENABLE_LOG}" STREQUAL "ON")
      set(DEFINES_JERRY ${DEFINES_JERRY} JERRY_ENABLE_LOG)
     endif()
    endif()

    set_property(TARGET ${TARGET_NAME}
                 PROPERTY COMPILE_FLAGS "${COMPILE_FLAGS_JERRY} ${CXX_FLAGS_JERRY} ${FLAGS_COMMON_${BUILD_MODE}}")
    set_property(TARGET ${TARGET_NAME}
                 PROPERTY LINK_FLAGS "${COMPILE_FLAGS_JERRY} ${CXX_FLAGS_JERRY} ${FLAGS_COMMON_${BUILD_MODE}} ${LINKER_FLAGS_COMMON} ${LINKER_FLAGS_STATIC}")
    target_compile_definitions(${TARGET_NAME} PRIVATE ${DEFINES_JERRY})
    target_include_directories(${TARGET_NAME} PRIVATE ${INCLUDE_CORE_INTERFACE})
    target_include_directories(${TARGET_NAME} SYSTEM PRIVATE ${INCLUDE_LIBC_INTERFACE})
    target_include_directories(${TARGET_NAME} SYSTEM PRIVATE ${INCLUDE_EXTERNAL_LIBS_INTERFACE})
    target_link_libraries(${TARGET_NAME} ${PLUGINS_TARGET_NAME} ${CORE_TARGET_NAME} ${LIBC_TARGET_NAME}
                          ${FDLIBM_TARGET_NAME} ${PREFIX_IMPORTED_LIB}libgcc ${PREFIX_IMPORTED_LIB}libgcc_eh)

    add_cppcheck_target(${TARGET_NAME})

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
   else()
    set(MAIN_MODULE_TARGET_NAME ${TARGET_NAME}.${PLATFORM_L}.main)
    add_library(${MAIN_MODULE_TARGET_NAME} ${SOURCE_JERRY_STANDALONE_MAIN})
    set_property(TARGET ${MAIN_MODULE_TARGET_NAME}
                 PROPERTY COMPILE_FLAGS "${COMPILE_FLAGS_JERRY} ${CXX_FLAGS_JERRY} ${FLAGS_COMMON_${BUILD_MODE}}")
    target_compile_definitions(${MAIN_MODULE_TARGET_NAME} PRIVATE ${DEFINES_JERRY})
    target_include_directories(${MAIN_MODULE_TARGET_NAME} PRIVATE ${INCLUDE_CORE_INTERFACE})
    target_include_directories(${MAIN_MODULE_TARGET_NAME} SYSTEM PRIVATE ${INCLUDE_LIBC_INTERFACE})
    target_include_directories(${MAIN_MODULE_TARGET_NAME} SYSTEM PRIVATE ${INCLUDE_EXTERNAL_LIBS_INTERFACE})

    add_custom_target(${TARGET_NAME} ALL)
    add_dependencies(${TARGET_NAME} ${MAIN_MODULE_TARGET_NAME} ${FDLIBM_TARGET_NAME} ${PLUGINS_TARGET_NAME} ${CORE_TARGET_NAME})

    add_custom_command(TARGET ${TARGET_NAME}
                       POST_BUILD
                       COMMAND mkdir -p ${CMAKE_BINARY_DIR}/${TARGET_NAME}
                       COMMAND echo
                       $<TARGET_FILE:${MAIN_MODULE_TARGET_NAME}>
                       $<TARGET_FILE:${FDLIBM_TARGET_NAME}>
                       $<TARGET_FILE:${PLUGINS_TARGET_NAME}>
                       $<TARGET_FILE:${CORE_TARGET_NAME}> > ${CMAKE_BINARY_DIR}/${TARGET_NAME}/list)

    if(${USE_JERRY_LIBC})
     add_dependencies(${TARGET_NAME} ${LIBC_TARGET_NAME})
    endif()

    add_cppcheck_target(${TARGET_NAME})
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
  if("${PLATFORM}" STREQUAL "LINUX")
   add_custom_target(unittests)
   add_custom_target(cppcheck.unittests)

   foreach(SOURCE_UNIT_TEST_MAIN ${SOURCE_UNIT_TEST_MAIN_MODULES})
    get_filename_component(TARGET_NAME ${SOURCE_UNIT_TEST_MAIN} NAME_WE)
    set(TARGET_NAME unit_${TARGET_NAME})

    set(CORE_TARGET_NAME unittests.jerry-core)
    set(FDLIBM_TARGET_NAME unittests.jerry-fdlibm${SUFFIX_THIRD_PARTY_LIB})

    add_executable(${TARGET_NAME} ${SOURCE_UNIT_TEST_MAIN})
    set_property(TARGET ${TARGET_NAME}
                 PROPERTY COMPILE_FLAGS "${COMPILE_FLAGS_JERRY} ${CXX_FLAGS_JERRY} ${FLAGS_COMMON_UNITTESTS}")
    set_property(TARGET ${TARGET_NAME}
                 PROPERTY LINK_FLAGS "${COMPILE_FLAGS_JERRY} ${CXX_FLAGS_JERRY} ${FLAGS_COMMON_UNITTESTS} ${LINKER_FLAGS_COMMON}")
    target_include_directories(${TARGET_NAME} PRIVATE ${INCLUDE_CORE_INTERFACE})
    target_link_libraries(${TARGET_NAME} ${CORE_TARGET_NAME} ${FDLIBM_TARGET_NAME} ${PREFIX_IMPORTED_LIB}libc
                          ${PREFIX_IMPORTED_LIB}libgcc ${PREFIX_IMPORTED_LIB}libgcc_eh)

    add_cppcheck_target(${TARGET_NAME})

    add_dependencies(unittests ${TARGET_NAME})
    add_dependencies(cppcheck.unittests cppcheck.${TARGET_NAME})
   endforeach()
  endif()
