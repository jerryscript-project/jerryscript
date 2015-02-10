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

# Remove rdynamic option
 set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS )
 set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS )

# Options
 option(STRIP_LINUX_RELEASE_BINARY "Strip symbols from Linux-targeted release binaries" ON)

# Build modes
 set(BUILD_MODES DEBUG RELEASE)

 # Debug
  set(BUILD_MODE_PREFIX_DEBUG debug)

 # Release
  set(BUILD_MODE_PREFIX_RELEASE release)

# Modifiers
 set(MODIFIERS
     COMPACT_PROFILE
     COMPACT_PROFILE_MINIMAL
     FULL_PROFILE
     MEMORY_STATISTICS
     VALGRIND)

 # Profiles
  # Full profile (default, so - no suffix)
   set(MODIFIER_SUFFIX_FULL_PROFILE )
   set(MODIFIER_DEFINES_FULL_PROFILE DEFINES_JERRY_FULL_PROFILE)
   set(MODIFIER_INCLUDE_DIRECTORIES_FULL_PROFILE )

  # Compact profile
   set(MODIFIER_SUFFIX_COMPACT_PROFILE -cp)
   set(MODIFIER_DEFINES_COMPACT_PROFILE DEFINES_JERRY_COMPACT_PROFILE)
   set(MODIFIER_INCLUDE_DIRECTORIES_COMPACT_PROFILE )

  # Minimal compact profile
   set(MODIFIER_SUFFIX_COMPACT_PROFILE_MINIMAL -cp_minimal)
   set(MODIFIER_DEFINES_COMPACT_PROFILE_MINIMAL DEFINES_JERRY_COMPACT_PROFILE_MINIMAL)
   set(MODIFIER_INCLUDE_DIRECTORIES_COMPACT_PROFILE_MINIMAL )

 # Memory statistics
  set(MODIFIER_SUFFIX_MEMORY_STATISTICS -mem_stats)
  set(MODIFIER_DEFINES_MEMORY_STATISTICS DEFINES_JERRY_MEMORY_STATS)
   set(MODIFIER_INCLUDE_DIRECTORIES_MEMORY_STATISTICS )

 # Valgrind
  set(MODIFIER_SUFFIX_VALGRIND -valgrind)
  set(MODIFIER_DEFINES_VALGRIND DEFINES_JERRY_VALGRIND)
  set(MODIFIER_INCLUDE_DIRECTORIES_VALGRIND INCLUDE_DIRECTORIES_THIRD_PARTY_VALGRIND)

# Compiler flags
 set(CXX_FLAGS_JERRY -std=c++11 -fno-exceptions -fno-rtti)

 # Turn off implicit template instantiation
  set(CXX_FLAGS_JERRY ${CXX_FLAGS_JERRY} -fno-implicit-templates -fno-implicit-inline-templates)

 # Turn off stack protector
  set(CXX_FLAGS_JERRY ${CXX_FLAGS_JERRY} -fno-stack-protector)

 # Debug information
  set(CXX_FLAGS_JERRY ${CXX_FLAGS_JERRY} -g -gdwarf-4)
 
 # Warnings
  set(CXX_FLAGS_JERRY ${CXX_FLAGS_JERRY} -Wall -Wextra -Wpedantic -Wlogical-op)
  set(CXX_FLAGS_JERRY ${CXX_FLAGS_JERRY} -Wformat-nonliteral -Winit-self -Wstack-protector)
  set(CXX_FLAGS_JERRY ${CXX_FLAGS_JERRY} -Wconversion -Wsign-conversion -Wformat-security)
  set(CXX_FLAGS_JERRY ${CXX_FLAGS_JERRY} -Wmissing-declarations -Wno-attributes)
  set(CXX_FLAGS_JERRY ${CXX_FLAGS_JERRY} -Werror -Wfatal-errors)

 # Architecture-specific
  # x86_64
   # Workaround for gcc bug 64905 (x86_64)
    set(CXX_FLAGS_JERRY_X86_64 -ffixed-rbp)

   # ARMv7
    set(CXX_FLAGS_JERRY_ARMV7 -mlittle-endian -mthumb)

 # Platform-specific
  # MCU
   # stm32f3
    set(CXX_FLAGS_COMMON_MCU_STM32F3 -mcpu=cortex-m4 -march=armv7e-m)
    set(CXX_FLAGS_COMMON_MCU_STM32F3 ${CXX_FLAGS_COMMON_MCU_STM32F3} -mfpu=fpv4-sp-d16 -mfloat-abi=hard)
   # stm32f4
    set(CXX_FLAGS_COMMON_MCU_STM32F4 -mcpu=cortex-m4 -march=armv7e-m)
    set(CXX_FLAGS_COMMON_MCU_STM32F4 ${CXX_FLAGS_COMMON_MCU_STM32F4} -mfpu=fpv4-sp-d16 -mfloat-abi=hard)

 # Debug
  set(CXX_FLAGS_COMMON_DEBUG )

 # Release
  set(CXX_FLAGS_COMMON_RELEASE -Os -flto)

 # Unit tests
  set(CXX_FLAGS_UNIT_TEST
      ${CXX_FLAGS_COMMON_RELEASE})

# Linker flags (flags are passed through set_target_properties,
#               so they should be specified withstring constant,
#               not list)
 set(LINKER_FLAGS_JERRY "-nostdlib -lgcc")

 # Static build
  set(LINKER_FLAGS_STATIC "-static")

 # Architecture-specific
  # x86_64
   # Workaround for gcc bug 64905 (x86_64)
    string(REPLACE ";" " " LINKER_FLAGS_JERRY_X86_64 "${CXX_FLAGS_JERRY_X86_64}")

   # ARMv7
    string(REPLACE ";" " " LINKER_FLAGS_JERRY_ARMV7 "${CXX_FLAGS_JERRY_ARMV7}")

# Platform-specific
  # MCU
   # stm32f3
    set(LINKER_FLAGS_JERRY_MCU_STM32F3 "-T${CMAKE_SOURCE_DIR}/third-party/stm32f3.ld")
   # stm32f4
    set(LINKER_FLAGS_JERRY_MCU_STM32F4 "-T${CMAKE_SOURCE_DIR}/third-party/stm32f4.ld")

 # Debug
  set(LINKER_FLAGS_COMMON_DEBUG "")

 # Release
  set(LINKER_FLAGS_COMMON_RELEASE "-Os -flto")

 # Unit tests
  set(LINKER_FLAGS_UNIT_TEST
      "${LINKER_FLAGS_COMMON_RELEASE}")

# Definitions
 # Common
  # Get version information from git
   execute_process(COMMAND git symbolic-ref -q HEAD
                   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                   OUTPUT_VARIABLE JERRY_GIT_BRANCH
                   OUTPUT_STRIP_TRAILING_WHITESPACE)
   execute_process(COMMAND git rev-parse HEAD
                   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                   OUTPUT_VARIABLE JERRY_GIT_COMMIT
                   OUTPUT_STRIP_TRAILING_WHITESPACE)
  # Get build date
   execute_process(COMMAND date +%d/%m/%Y
                   OUTPUT_VARIABLE JERRY_BUILD_DATE
                   OUTPUT_STRIP_TRAILING_WHITESPACE)

   set(DEFINES_JERRY
       JERRY_BUILD_DATE="${JERRY_BUILD_DATE}"
       JERRY_COMMIT_HASH="${JERRY_GIT_COMMIT}"
       JERRY_BRANCH_NAME="${JERRY_GIT_BRANCH}")

  # Debug
   set(DEFINES_JERRY_DEBUG )

  # Release
   set(DEFINES_JERRY_RELEASE JERRY_NDEBUG)

  # Full profile
   set(DEFINES_JERRY_FULL_PROFILE CONFIG_ECMA_NUMBER_TYPE=CONFIG_ECMA_NUMBER_FLOAT64)

  # Compact profile
   set(DEFINES_JERRY_COMPACT_PROFILE
       CONFIG_ECMA_COMPACT_PROFILE)

  # Minimal compact profile
   set(DEFINES_JERRY_COMPACT_PROFILE_MINIMAL
       CONFIG_ECMA_COMPACT_PROFILE
       CONFIG_ECMA_COMPACT_PROFILE_DISABLE_NUMBER_BUILTIN
       CONFIG_ECMA_COMPACT_PROFILE_DISABLE_STRING_BUILTIN
       CONFIG_ECMA_COMPACT_PROFILE_DISABLE_BOOLEAN_BUILTIN
       CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ERROR_BUILTINS
       CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ARRAY_BUILTIN
       CONFIG_ECMA_COMPACT_PROFILE_DISABLE_MATH_BUILTIN
       CONFIG_ECMA_COMPACT_PROFILE_DISABLE_DATE_BUILTIN
       CONFIG_ECMA_COMPACT_PROFILE_DISABLE_JSON_BUILTIN
       CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN)

 # Memory statistics
  set(DEFINES_JERRY_MEMORY_STATS MEM_STATS)

 # Valgrind
  set(DEFINES_JERRY_VALGRIND JERRY_VALGRIND)

 # Architecture-specific
  # x86_64
   set(DEFINES_JERRY_X86_64 __TARGET_HOST_x64)
  # ARMv7
   set(DEFINES_JERRY_ARMV7 __TARGET_HOST_ARMv7)

 # Platform-specific
  # Linux
   set(DEFINES_JERRY_LINUX __TARGET_HOST JERRY_SOURCE_BUFFER_SIZE=1048576)
  # MCU
   set(DEFINES_JERRY_MCU __TARGET_MCU)
   # stm32f3
    set(DEFINES_JERRY_MCU_STM32F3 __TARGET_MCU_STM32F3)
   # stm32f4
    set(DEFINES_JERRY_MCU_STM32F4 __TARGET_MCU_STM32F4)

 # Unit tests
  set(DEFINES_UNIT_TEST
      ${DEFINES_JERRY_FULL_PROFILE}
      ${DEFINES_JERRY_DEBUG}
      ${DEFINES_JERRY_VALGRIND})

# Include directories
 set(INCLUDE_DIRECTORIES_JERRY
     src
     src/liballocator
     src/libcoreint
     src/libecmabuiltins
     src/libecmaobjects
     src/libecmaoperations
     src/libintstructs
     src/libjsparser
     src/liboptimizer
     src/libperipherals
     src/libruntime)

 # Platform-specific
  # Linux
   set(INCLUDE_DIRECTORIES_JERRY_LINUX
       src/libruntime/target/linux)
  # MCU
   # STM32F3
    set(INCLUDE_DIRECTORIES_JERRY_MCU_STM32F3
        src/libruntime/target/stm32f3)
   # STM32F4
    set(INCLUDE_DIRECTORIES_JERRY_MCU_STM32F4
        src/libruntime/target/stm32f4)

 # Third-party
  # Valgrind
   set(INCLUDE_DIRECTORIES_THIRD_PARTY_VALGRIND third-party/valgrind)

  # Platform-specific
   # MCU
    # STM32F3
     set(INCLUDE_DIRECTORIES_THIRD_PARTY_MCU_STM32F3
         third-party/STM32F3-Discovery_FW_V1.1.0/Libraries/CMSIS/Device/ST/STM32F30x/Include
         third-party/STM32F3-Discovery_FW_V1.1.0/Libraries/STM32F30x_StdPeriph_Driver/inc
         third-party/STM32F3-Discovery_FW_V1.1.0/Libraries/CMSIS/Include
         third-party/STM32F3-Discovery_FW_V1.1.0)
    # STM32F4
     set(INCLUDE_DIRECTORIES_THIRD_PARTY_MCU_STM32F4
 	 third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/CMSIS/ST/STM32F4xx/Include
 	 third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/STM32F4xx_StdPeriph_Driver/inc
 	 third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/CMSIS/Include
 	 third-party/STM32F4-Discovery_FW_V1.1.0)

 # Unit tests
  set(INCLUDE_DIRECTORIES_UNIT_TEST
      tests/unit
      ${INCLUDE_DIRECTORIES_THIRD_PARTY_VALGRIND})

# Sources
 # Jerry
  set(SOURCE_JERRY_LIB_ALLOCATOR # liballocator
      src/liballocator/mem-allocator.cpp
      src/liballocator/mem-heap.cpp
      src/liballocator/mem-pool.cpp
      src/liballocator/mem-poolman.cpp)
 
  set(SOURCE_JERRY_LIB_COREINT # libcoreint
      src/libcoreint/interpreter.cpp
      src/libcoreint/opcodes-agnostic.cpp
      src/libcoreint/opcodes.cpp
      src/libcoreint/opcodes-ecma-arithmetics.cpp
      src/libcoreint/opcodes-ecma-bitwise.cpp
      src/libcoreint/opcodes-ecma-equality.cpp
      src/libcoreint/opcodes-ecma-relational.cpp
      src/libcoreint/opcodes-ecma-try-catch-finally.cpp
      src/libcoreint/opcodes-helpers-variables.cpp
      src/libcoreint/opcodes-native-call.cpp
      src/libcoreint/opcodes-varg.cpp)
 
  set(SOURCE_JERRY_LIB_ECMABUILTINS # libecmabuiltins
      src/libecmabuiltins/ecma-builtin-array.cpp
      src/libecmabuiltins/ecma-builtin-array-prototype.cpp
      src/libecmabuiltins/ecma-builtin-boolean.cpp
      src/libecmabuiltins/ecma-builtin-boolean-prototype.cpp
      src/libecmabuiltins/ecma-builtin-compact-profile-error.cpp
      src/libecmabuiltins/ecma-builtin-error.cpp
      src/libecmabuiltins/ecma-builtin-error-prototype.cpp
      src/libecmabuiltins/ecma-builtin-evalerror.cpp
      src/libecmabuiltins/ecma-builtin-evalerror-prototype.cpp
      src/libecmabuiltins/ecma-builtin-function.cpp
      src/libecmabuiltins/ecma-builtin-function-prototype.cpp
      src/libecmabuiltins/ecma-builtin-global.cpp
      src/libecmabuiltins/ecma-builtin-math.cpp
      src/libecmabuiltins/ecma-builtin-number.cpp
      src/libecmabuiltins/ecma-builtin-number-prototype.cpp
      src/libecmabuiltins/ecma-builtin-object.cpp
      src/libecmabuiltins/ecma-builtin-object-prototype.cpp
      src/libecmabuiltins/ecma-builtin-rangeerror.cpp
      src/libecmabuiltins/ecma-builtin-rangeerror-prototype.cpp
      src/libecmabuiltins/ecma-builtin-referenceerror.cpp
      src/libecmabuiltins/ecma-builtin-referenceerror-prototype.cpp
      src/libecmabuiltins/ecma-builtins.cpp
      src/libecmabuiltins/ecma-builtin-string.cpp
      src/libecmabuiltins/ecma-builtin-string-prototype.cpp
      src/libecmabuiltins/ecma-builtin-syntaxerror.cpp
      src/libecmabuiltins/ecma-builtin-syntaxerror-prototype.cpp
      src/libecmabuiltins/ecma-builtin-typeerror.cpp
      src/libecmabuiltins/ecma-builtin-typeerror-prototype.cpp
      src/libecmabuiltins/ecma-builtin-type-error-thrower.cpp
      src/libecmabuiltins/ecma-builtin-urierror.cpp
      src/libecmabuiltins/ecma-builtin-urierror-prototype.cpp)
 
  set(SOURCE_JERRY_LIB_ECMAOBJECTS # libecmaobjects
      src/libecmaobjects/ecma-alloc.cpp
      src/libecmaobjects/ecma-gc.cpp
      src/libecmaobjects/ecma-helpers.cpp
      src/libecmaobjects/ecma-helpers-conversion.cpp
      src/libecmaobjects/ecma-helpers-number.cpp
      src/libecmaobjects/ecma-helpers-string.cpp
      src/libecmaobjects/ecma-helpers-value.cpp
      src/libecmaobjects/ecma-helpers-values-collection.cpp
      src/libecmaobjects/ecma-lcache.cpp
      src/libecmaobjects/ecma-stack.cpp)
 
  set(SOURCE_JERRY_LIB_ECMAOPERATIONS # libecmaoperations
      src/libecmaoperations/ecma-array-object.cpp
      src/libecmaoperations/ecma-boolean-object.cpp
      src/libecmaoperations/ecma-comparison.cpp
      src/libecmaoperations/ecma-conversion.cpp
      src/libecmaoperations/ecma-exceptions.cpp
      src/libecmaoperations/ecma-function-object.cpp
      src/libecmaoperations/ecma-get-put-value.cpp
      src/libecmaoperations/ecma-init-finalize.cpp
      src/libecmaoperations/ecma-lex-env.cpp
      src/libecmaoperations/ecma-number-arithmetic.cpp
      src/libecmaoperations/ecma-number-object.cpp
      src/libecmaoperations/ecma-objects-arguments.cpp
      src/libecmaoperations/ecma-objects.cpp
      src/libecmaoperations/ecma-objects-general.cpp
      src/libecmaoperations/ecma-reference.cpp
      src/libecmaoperations/ecma-string-object.cpp)
 
  set(SOURCE_JERRY_LIB_INTSTRUCTS # libintstructs
      src/libintstructs/array-list.cpp
      src/libintstructs/hash-table.cpp
      src/libintstructs/linked-list.cpp
      src/libintstructs/literal.cpp
      src/libintstructs/lp-string.cpp)
 
  set(SOURCE_JERRY_LIB_JSPARSER # libjsparser
      src/libjsparser/lexer.cpp
      src/libjsparser/opcodes-dumper.cpp
      src/libjsparser/parser.cpp
      src/libjsparser/scopes-tree.cpp
      src/libjsparser/syntax-errors.cpp)
 
  set(SOURCE_JERRY_LIB_OPTIMIZER # liboptimizer
      src/liboptimizer/deserializer.cpp
      src/liboptimizer/lit-id-hash-table.cpp
      src/liboptimizer/optimizer-passes.cpp
      src/liboptimizer/pretty-printer.cpp
      src/liboptimizer/serializer.cpp)
 
  set(SOURCE_JERRY_LIB_PERIPHERALS # libperipherals
      src/libperipherals/actuators.cpp
      src/libperipherals/common-io.cpp
      src/libperipherals/sensors.cpp)
 
  set(SOURCE_JERRY_LIB_RUNTIME # libruntime
      src/libruntime/jerry-exit.cpp
      src/libruntime/jerry-libc.cpp
      src/libruntime/jerry-libc-printf.cpp
      src/libruntime/jrt-bit-fields.cpp)

  set(SOURCE_JERRY_LIB # all Jerry library's source
                       # that is not platform-specific
      ${SOURCE_JERRY_LIB_ALLOCATOR}
      ${SOURCE_JERRY_LIB_COREINT}
      ${SOURCE_JERRY_LIB_ECMABUILTINS}
      ${SOURCE_JERRY_LIB_ECMAOBJECTS}
      ${SOURCE_JERRY_LIB_ECMAOPERATIONS}
      ${SOURCE_JERRY_LIB_INTSTRUCTS}
      ${SOURCE_JERRY_LIB_JSPARSER}
      ${SOURCE_JERRY_LIB_OPTIMIZER}
      ${SOURCE_JERRY_LIB_PERIPHERALS}
      ${SOURCE_JERRY_LIB_RUNTIME})
  
  set(SOURCE_JERRY_MAIN # main Jerry module
      src/main.cpp)

  # Platform-specific
   # Linux
    set(SOURCE_JERRY_LIB_RUNTIME_LINUX
        src/libruntime/target/linux/jerry-libc.cpp
        src/libruntime/target/linux/jerry-assert.cpp
        src/libruntime/target/linux/jerry-asm.S)
   # MCU
    # stm32f3
     set(SOURCE_JERRY_LIB_RUNTIME_STM32F3
         src/libruntime/target/stm32f3/jerry-libc.cpp
         src/libruntime/target/stm32f3/jerry-assert.cpp
         src/libruntime/target/stm32f3/abi.S)
    # stm32f4
     set(SOURCE_JERRY_LIB_RUNTIME_STM32F4
         src/libruntime/target/stm32f4/jerry-libc.cpp
         src/libruntime/target/stm32f4/jerry-assert.cpp
         src/libruntime/target/stm32f4/abi.S)

 # Third-party
  # Platform-specific
   # MCU
    # stm32f3
     set(SOURCE_THIRD_PARTY_MCU_STM32F3
         third-party/STM32F3-Discovery_FW_V1.1.0/Libraries/CMSIS/Device/ST/STM32F30x/Source/Templates/gcc_ride7/startup_stm32f30x.s
         third-party/STM32F3-Discovery_FW_V1.1.0/Libraries/CMSIS/Device/ST/STM32F30x/Source/Templates/system_stm32f30x.c
         third-party/STM32F3-Discovery_FW_V1.1.0/Libraries/STM32F30x_StdPeriph_Driver/src/stm32f30x_tim.c
         third-party/STM32F3-Discovery_FW_V1.1.0/Libraries/STM32F30x_StdPeriph_Driver/src/stm32f30x_gpio.c
         third-party/STM32F3-Discovery_FW_V1.1.0/Libraries/STM32F30x_StdPeriph_Driver/src/stm32f30x_rcc.c)
    # stm32f4
     set(SOURCE_THIRD_PARTY_MCU_STM32F4
         third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/CMSIS/ST/STM32F4xx/Source/Templates/gcc_ride7/startup_stm32f4xx.s
         third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/CMSIS/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c
         third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_tim.c
         third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_gpio.c
         third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_rcc.c)

 # Unit tests main modules
  set(SOURCE_UNIT_TEST_MAIN_MODULES
      tests/unit/test_heap.cpp
      tests/unit/test_number_to_string.cpp
      tests/unit/test_pool.cpp
      tests/unit/test_poolman.cpp
      tests/unit/test_preparser.cpp
      tests/unit/test_string_to_number.cpp)

# Imported libraries
 # libc
  add_library(imported_libc SHARED IMPORTED)
  execute_process(COMMAND gcc -print-file-name=libc.so
                  OUTPUT_VARIABLE IMPORTED_LIBC_LOCATION
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  set_property(TARGET imported_libc
               PROPERTY IMPORTED_LOCATION ${IMPORTED_LIBC_LOCATION})
 # libgcc
  add_library(imported_libgcc STATIC IMPORTED)
  execute_process(COMMAND gcc -print-file-name=libgcc.a
                  OUTPUT_VARIABLE IMPORTED_LIBGCC_LOCATION
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  set_property(TARGET imported_libgcc
               PROPERTY IMPORTED_LOCATION ${IMPORTED_LIBGCC_LOCATION})

# Architecture-specific configuration
 if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
  set(CXX_FLAGS_COMMON_ARCH ${CXX_FLAGS_JERRY_X86_64})
  set(LINKER_FLAGS_JERRY "${LINKER_FLAGS_JERRY} ${LINKER_FLAGS_JERRY_X86_64}")
  set(DEFINES_JERRY ${DEFINES_JERRY} ${DEFINES_JERRY_X86_64})
 elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "armv7l")
  set(CXX_FLAGS_COMMON_ARCH ${CXX_FLAGS_JERRY_ARMV7})
  set(LINKER_FLAGS_JERRY "${LINKER_FLAGS_JERRY} ${LINKER_FLAGS_JERRY_ARMV7}")
  set(DEFINES_JERRY ${DEFINES_JERRY} ${DEFINES_JERRY_ARMV7})
 else()
  message(FATAL_ERROR "Unsupported machine architecture")
 endif()

# Platform-specific configuration
 if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(DEFINES_JERRY ${DEFINES_JERRY} ${DEFINES_JERRY_LINUX})
 elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Generic")
  set(DEFINES_JERRY ${DEFINES_JERRY} ${DEFINES_JERRY_MCU})
 else()
  message(FATAL_ERROR "Platform is not supported")
 endif()

# Targets declaration
 if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  function(declare_targets_for_build_mode BUILD_MODE MODIFIERS_LISTS)
   set(TARGET_NAME ${BUILD_MODE_PREFIX_${BUILD_MODE}}.linux)
   set(DEFINES_JERRY ${DEFINES_JERRY} ${DEFINES_JERRY_${BUILD_MODE}})
   set(LINKER_FLAGS_JERRY "${LINKER_FLAGS_JERRY} ${LINKER_FLAGS_COMMON_${BUILD_MODE}}")

   function(declare_target_with_modifiers ) # modifiers are passed in ARGN implicit argument
    foreach(MODIFIER ${ARGN})
     set(TARGET_NAME ${TARGET_NAME}${MODIFIER_SUFFIX_${MODIFIER}})
     set(DEFINES_JERRY ${DEFINES_JERRY} ${${MODIFIER_DEFINES_${MODIFIER}}})
     set(INCLUDE_DIRECTORIES_JERRY ${INCLUDE_DIRECTORIES_JERRY} ${${MODIFIER_INCLUDE_DIRECTORIES_${MODIFIER}}})
    endforeach()

    add_library(${TARGET_NAME}.lib STATIC ${SOURCE_JERRY_LIB} ${SOURCE_JERRY_LIB_RUNTIME_LINUX})
    target_compile_options(${TARGET_NAME}.lib PRIVATE ${CXX_FLAGS_JERRY} ${CXX_FLAGS_COMMON_ARCH} ${CXX_FLAGS_COMMON_${BUILD_MODE}})
    target_compile_definitions(${TARGET_NAME}.lib PRIVATE ${DEFINES_JERRY})
    target_include_directories(${TARGET_NAME}.lib PRIVATE ${INCLUDE_DIRECTORIES_JERRY})

    add_executable(${TARGET_NAME} ${SOURCE_JERRY_MAIN})
    target_compile_options(${TARGET_NAME} PRIVATE ${CXX_FLAGS_JERRY} ${CXX_FLAGS_COMMON_ARCH} ${CXX_FLAGS_COMMON_${BUILD_MODE}})
    set_property(TARGET ${TARGET_NAME} PROPERTY LINK_FLAGS "${LINKER_FLAGS_JERRY} ${LINKER_FLAGS_STATIC}")
    target_compile_definitions(${TARGET_NAME} PRIVATE ${DEFINES_JERRY})
    target_include_directories(${TARGET_NAME} PRIVATE ${INCLUDE_DIRECTORIES_JERRY})
    target_link_libraries(${TARGET_NAME} ${TARGET_NAME}.lib imported_libgcc)

    if(${STRIP_LINUX_RELEASE_BINARY} STREQUAL "ON")
     add_custom_command(TARGET ${TARGET_NAME}
                        POST_BUILD
                        COMMAND ${CMAKE_STRIP} $<TARGET_FILE:${TARGET_NAME}>)
    endif()
   endfunction()

   foreach(MODIFIERS_LIST ${MODIFIERS_LISTS})
    separate_arguments(MODIFIERS_LIST)

    declare_target_with_modifiers(${MODIFIERS_LIST})
   endforeach()
  endfunction()

  set(DEBUG_AND_RELEASE_MODIFIERS_LISTS
      "FULL_PROFILE"
      "COMPACT_PROFILE"
      "COMPACT_PROFILE_MINIMAL"
      "FULL_PROFILE MEMORY_STATISTICS"
      "COMPACT_PROFILE_MINIMAL MEMORY_STATISTICS"
      "FULL_PROFILE VALGRIND"
      "COMPACT_PROFILE_MINIMAL VALGRIND"
      "COMPACT_PROFILE VALGRIND")
  declare_targets_for_build_mode(DEBUG "${DEBUG_AND_RELEASE_MODIFIERS_LISTS}")
  declare_targets_for_build_mode(RELEASE "${DEBUG_AND_RELEASE_MODIFIERS_LISTS}")

  # Unit tests declaration
   add_custom_target(unittests)

   add_library(unit_tests.lib STATIC ${SOURCE_JERRY_LIB} ${SOURCE_JERRY_LIB_RUNTIME_LINUX})
   target_compile_options(unit_tests.lib PRIVATE ${CXX_FLAGS_JERRY} ${CXX_FLAGS_COMMON_ARCH} ${CXX_FLAGS_UNIT_TEST})
   target_compile_definitions(unit_tests.lib PRIVATE ${DEFINES_JERRY} ${DEFINES_UNIT_TEST})
   target_include_directories(unit_tests.lib PRIVATE ${INCLUDE_DIRECTORIES_JERRY} ${INCLUDE_DIRECTORIES_UNIT_TEST})

   foreach(SOURCE_UNIT_TEST_MAIN ${SOURCE_UNIT_TEST_MAIN_MODULES})
    get_filename_component(TARGET_NAME ${SOURCE_UNIT_TEST_MAIN} NAME_WE)
    set(TARGET_NAME unit_${TARGET_NAME})

    add_executable(${TARGET_NAME} ${SOURCE_UNIT_TEST_MAIN})
    target_compile_options(${TARGET_NAME} PRIVATE
                           ${CXX_FLAGS_JERRY}
                           ${CXX_FLAGS_COMMON_ARCH}
                           ${CXX_FLAGS_UNIT_TEST})
    set_property(TARGET ${TARGET_NAME} PROPERTY LINK_FLAGS "${LINKER_FLAGS_JERRY} ${LINKER_FLAGS_UNIT_TEST}")
    target_compile_definitions(${TARGET_NAME} PRIVATE ${DEFINES_JERRY} ${DEFINES_UNIT_TEST})
    target_include_directories(${TARGET_NAME} PRIVATE ${INCLUDE_DIRECTORIES_JERRY} ${INCLUDE_DIRECTORIES_UNIT_TEST})
    target_link_libraries(${TARGET_NAME} unit_tests.lib imported_libc imported_libgcc)

    add_dependencies(unittests ${TARGET_NAME})
   endforeach()
 elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Generic")
  function(declare_target_for_platform PLATFORM) # build modes are passed in ARGN
   string(TOLOWER ${PLATFORM} PLATFORM_L)
   set(TARGET_NAME ${PLATFORM_L})
   set(BUILD_MODES_FOR_PLATFORM ${ARGN})

   function(declare_targets_for_build_mode BUILD_MODE)
    set(TARGET_NAME ${BUILD_MODE_PREFIX_${BUILD_MODE}}.${TARGET_NAME})
    set(DEFINES_JERRY ${DEFINES_JERRY} ${DEFINES_JERRY_${BUILD_MODE}})
    set(LINKER_FLAGS_JERRY "${LINKER_FLAGS_JERRY} ${LINKER_FLAGS_COMMON_${BUILD_MODE}}")

    function(declare_target_with_modifiers ) # modifiers are passed in ARGN
     foreach(MODIFIER ${ARGN})
      set(TARGET_NAME ${TARGET_NAME}${MODIFIER_SUFFIX_${MODIFIER}})
      set(DEFINES_JERRY ${DEFINES_JERRY} ${${MODIFIER_DEFINES_${MODIFIER}}})
      set(INCLUDE_DIRECTORIES_JERRY ${INCLUDE_DIRECTORIES_JERRY} ${${MODIFIER_INCLUDE_DIRECTORIES_${MODIFIER}}})
     endforeach()

     # Jerry library
      add_library(${TARGET_NAME}.jerry.lib STATIC ${SOURCE_JERRY_LIB} ${SOURCE_JERRY_LIB_RUNTIME_${PLATFORM}})
      target_compile_options(${TARGET_NAME}.jerry.lib PRIVATE
                             ${CXX_FLAGS_JERRY}
                             ${CXX_FLAGS_COMMON_${BUILD_MODE}}
                             ${CXX_FLAGS_COMMON_ARCH}
                             ${CXX_FLAGS_COMMON_MCU_${PLATFORM}})
      target_compile_definitions(${TARGET_NAME}.jerry.lib PRIVATE ${DEFINES_JERRY} ${DEFINES_JERRY_MCU_${PLATFORM}})
      target_include_directories(${TARGET_NAME}.jerry.lib PRIVATE
                                 ${INCLUDE_DIRECTORIES_JERRY}
                                 ${INCLUDE_DIRECTORIES_JERRY_MCU_${PLATFORM}}
                                 ${INCLUDE_DIRECTORIES_THIRD_PARTY_MCU_${PLATFORM}})
   
     # Third-party MCU library
      add_library(${TARGET_NAME}.third_party.lib STATIC
                  ${SOURCE_THIRD_PARTY_MCU_${PLATFORM}})
      target_compile_options(${TARGET_NAME}.third_party.lib PRIVATE
                             ${CXX_FLAGS_COMMON_${BUILD_MODE}}
                             ${CXX_FLAGS_COMMON_ARCH}
                             ${CXX_FLAGS_COMMON_MCU_${PLATFORM}})
      target_include_directories(${TARGET_NAME}.third_party.lib PRIVATE
                                 ${INCLUDE_DIRECTORIES_THIRD_PARTY_MCU_${PLATFORM}})

     add_executable(${TARGET_NAME} ${SOURCE_JERRY_MAIN})
     target_compile_options(${TARGET_NAME} PRIVATE
                            ${CXX_FLAGS_JERRY}
                            ${CXX_FLAGS_COMMON_${BUILD_MODE}}
                            ${CXX_FLAGS_COMMON_ARCH}
                            ${CXX_FLAGS_COMMON_MCU_${PLATFORM}})
     set_property(TARGET ${TARGET_NAME} PROPERTY LINK_FLAGS
                  "${LINKER_FLAGS_JERRY} ${LINKER_FLAGS_STATIC} ${LINKER_FLAGS_JERRY_MCU_${PLATFORM}}")
     target_compile_definitions(${TARGET_NAME} PRIVATE ${DEFINES_JERRY} ${DEFINES_JERRY_MCU_${PLATFORM}})
     target_include_directories(${TARGET_NAME} PRIVATE
                                ${INCLUDE_DIRECTORIES_JERRY}
                                ${INCLUDE_DIRECTORIES_JERRY_MCU_${PLATFORM}}
                                ${INCLUDE_DIRECTORIES_THIRD_PARTY_MCU_${PLATFORM}})
     target_link_libraries(${TARGET_NAME} ${TARGET_NAME}.jerry.lib ${TARGET_NAME}.third_party.lib imported_libgcc)

     add_custom_target(${TARGET_NAME}.flash
                       COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${TARGET_NAME}> $<TARGET_FILE:${TARGET_NAME}>.bin
                       # Flash, ignoring failures to remove temporary flashable binary any way
                       COMMAND st-flash write $<TARGET_FILE:${TARGET_NAME}>.bin 0x08000000 || /bin/true
                       COMMAND rm $<TARGET_FILE:${TARGET_NAME}>.bin)
    endfunction()

    declare_target_with_modifiers(COMPACT_PROFILE)
    declare_target_with_modifiers(COMPACT_PROFILE_MINIMAL)
   endfunction()

   foreach(BUILD_MODE ${BUILD_MODES_FOR_PLATFORM})
    declare_targets_for_build_mode(${BUILD_MODE})
   endforeach()
  endfunction()

  declare_target_for_platform(STM32F3 RELEASE)
  declare_target_for_platform(STM32F4 DEBUG RELEASE)
 else()
  message(FATAL_ERROR "Platform is not supported")
 endif()
