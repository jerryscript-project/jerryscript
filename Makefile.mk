ifeq ($(TARGET),)
  $(error TARGET not set)
endif

ENGINE_NAME ?= jerry

CROSS_COMPILE ?= arm-none-eabi-
CC  = gcc
LD  = ld
OBJDUMP = objdump
OBJCOPY = objcopy
SIZE = size
STRIP = strip

MAIN_MODULE_SRC = ./src/main.c

LNK_SCRIPT_STM32F4 = ./third-party/stm32f4.ld

# Parsing target
#   '.' -> ' '
TARGET_SPACED := $(subst ., ,$(TARGET))
#   extract target mode part
TARGET_MODE   := $(word 1,$(TARGET_SPACED))
#   extract target system part with modifiers
TARGET_SYSTEM_AND_MODS := $(word 2,$(TARGET_SPACED))
TARGET_SYSTEM_AND_MODS_SPACED := $(subst -, ,$(TARGET_SYSTEM_AND_MODS))

#   extract target system part
TARGET_SYSTEM := $(word 1,$(TARGET_SYSTEM_AND_MODS_SPACED))

#   extract modifiers
TARGET_MODS := $(wordlist 2, $(words $(TARGET_SYSTEM_AND_MODS_SPACED)), $(TARGET_SYSTEM_AND_MODS_SPACED))

#   extract optional action part
TARGET_ACTION := $(word 3,$(TARGET_SPACED))

# Target used as dependency of an action (check, flash, etc.)
TARGET_OF_ACTION := $(TARGET_MODE).$(TARGET_SYSTEM_AND_MODS)

# unittests mode -> linux system
ifeq ($(TARGET_MODE),$(TESTS_TARGET))
 TARGET_SYSTEM := linux
 TARGET_SYSTEM_AND_MODS := $(TARGET_SYSTEM)
endif

# target folder name in $(OUT_DIR)
TARGET_DIR=$(OUT_DIR)/$(TARGET_MODE).$(TARGET_SYSTEM_AND_MODS)

#
# Options setup
#

# Is MCU target?
ifeq ($(filter-out $(TARGET_MCU_SYSTEMS),$(TARGET_SYSTEM)),)
	OPTION_MCU = enable
else
	OPTION_MCU = disable
endif

# DWARF version
ifeq ($(dwarf4),1)
    OPTION_DWARF4 := enable
else
    OPTION_DWARF4 := disable
endif

# Print TODO, FIXME
ifeq ($(todo),1)
    OPTION_TODO := enable
else
    OPTION_TODO := disable
endif

ifeq ($(fixme),1)
    OPTION_FIXME := enable
else
    OPTION_FIXME := disable
endif

# Compilation command line echoing
ifeq ($(echo),1)
     OPTION_ECHO := enable
else
     OPTION_ECHO := disable
endif

# -fdiagnostics-color=always
ifeq ($(color),1)
     ifeq ($(OPTION_MCU),enable)
      $(error MCU target doesn\'t support coloring compiler's output)
     endif

     OPTION_COLOR := enable
else
     OPTION_COLOR := disable
endif

# JERRY_NDEBUG, debug symbols
ifeq ($(TARGET_MODE),release)
 OPTION_NDEBUG = enable
 OPTION_DEBUG_SYMS = disable
 OPTION_STRIP = enable
else
 OPTION_NDEBUG = disable
 OPTION_DEBUG_SYMS = enable
 OPTION_STRIP = disable
endif

# Optimizations
ifeq ($(filter-out debug_release release $(TESTS_TARGET),$(TARGET_MODE)),)
 OPTION_OPTIMIZE = enable
else
 OPTION_OPTIMIZE = disable
endif

ifeq ($(filter musl,$(TARGET_MODS)), musl)
     ifeq ($(OPTION_MCU),enable)
      $(error MCU target doesn\'t support LIBC_MUSL)
     endif

     OPTION_LIBC := musl
else
     OPTION_LIBC := raw
endif

ifeq ($(filter sanitize,$(TARGET_MODS)), sanitize)
     ifeq ($(OPTION_LIBC),musl)
      $(error ASAN and LIBC_MUSL are mutually exclusive)
     endif

     OPTION_SANITIZE := enable
else
     OPTION_SANITIZE := disable
endif

ifeq ($(filter valgrind,$(TARGET_MODS)), valgrind)
     OPTION_VALGRIND := enable

     ifeq ($(OPTION_SANITIZE),enable)
      $(error ASAN and Valgrind are mutually exclusive)
     endif
else
     OPTION_VALGRIND := disable
endif

#
# Target CPU
#
TARGET_CPU = $(strip $(if $(filter linux,$(TARGET_SYSTEM)), x64, \
                     $(if $(filter stm32f3,$(TARGET_SYSTEM)), cortexm4, \
                     $(if $(filter stm32f4,$(TARGET_SYSTEM)), cortexm4, \
                     $(error Do not know target CPU for target system '$(TARGET_SYSTEM)')))))

#
# Flag blocks
#

# Warnings
CFLAGS_WARNINGS ?= -Wall -Wextra -Wpedantic -Wlogical-op -Winline \
                   -Wformat-nonliteral -Winit-self -Wstack-protector \
                   -Wconversion -Wsign-conversion -Wformat-security \
                   -Wstrict-prototypes -Wmissing-prototypes
CFLAGS_WERROR ?= -Werror
CFLAGS_WFATAL_ERRORS ?= -Wfatal-errors

# Optimizations
CFLAGS_OPTIMIZE ?= -O3 -flto
CFLAGS_NO_OPTIMIZE ?= -O0
LDFLAGS_OPTIMIZE ?=
LDFLAGS_NO_OPTIMIZE ?=

# Debug symbols
CFLAGS_DEBUG_SYMS ?= -g3

ifeq ($(OPTION_DWARF4),enable)
     CFLAGS_DEBUG_SYMS += -gdwarf-4
else
     CFLAGS_DEBUG_SYMS += -gdwarf-3
endif

# Cortex-M4 MCU
CFLAGS_CORTEXM4 ?= -mlittle-endian -mcpu=cortex-m4 -march=armv7e-m -mthumb \
		   -mfpu=fpv4-sp-d16 -mfloat-abi=hard 


#
# Common
#

CFLAGS_COMMON ?= $(INCLUDES) -std=c99

LDFLAGS ?= 

ifeq ($(OPTION_OPTIMIZE),enable)
 CFLAGS_COMMON += $(CFLAGS_OPTIMIZE)
 LDFLAGS += $(LDFLAGS_OPTIMIZE)
else
 CFLAGS_COMMON += $(CFLAGS_NO_OPTIMIZE)
 LDFLAGS += $(LDFLAGS_NO_OPTIMIZE)
endif

ifeq ($(OPTION_DEBUG_SYMS),enable)
 CFLAGS_COMMON += $(CFLAGS_DEBUG_SYMS)
endif

# CPU-specific common
ifeq ($(TARGET_CPU),cortexm4)
 CFLAGS_COMMON += $(CFLAGS_CORTEXM4)
endif

# System-specific common
ifeq ($(TARGET_SYSTEM),stm32f4)
 LDFLAGS += -nostartfiles -T$(LNK_SCRIPT_STM32F4)
endif

ifeq ($(OPTION_MCU),enable)
 CC := $(CROSS_COMPILE)$(CC)
 LD := $(CROSS_COMPILE)$(LD)
 OBJDUMP := $(CROSS_COMPILE)$(OBJDUMP)
 OBJCOPY := $(CROSS_COMPILE)$(OBJCOPY)
 SIZE := $(CROSS_COMPILE)$(SIZE)
 STRIP := $(CROSS_COMPILE)$(STRIP)
endif

#
# Jerry part sources, headers, includes, cflags, ldflags
#

GIT_BRANCH=$(shell git symbolic-ref -q HEAD)
GIT_HASH=$(shell git rev-parse HEAD)
BUILD_DATE=$(shell date +'%d/%m/%Y')

CFLAGS_JERRY = $(CFLAGS_WARNINGS) $(CFLAGS_WERROR) $(CFLAGS_WFATAL_ERRORS)
DEFINES_JERRY = -DMEM_HEAP_CHUNK_SIZE=$$((64)) -DMEM_HEAP_AREA_SIZE=$$((2 * 1024 + 512)) -DMEM_STATS \
                -DCONFIG_ECMA_REFERENCE_COUNTER_WIDTH=$$((10))

DEFINES_JERRY += -DJERRY_BUILD_DATE="\"$(BUILD_DATE)\"" \
                 -DJERRY_COMMIT_HASH="\"$(GIT_HASH)\"" \
                 -DJERRY_BRANCH_NAME="\"$(GIT_BRANCH)\""

SOURCES_JERRY_C = \
 $(sort \
 $(wildcard ./src/libruntime/*.c) \
 $(wildcard ./src/libperipherals/*.c) \
 $(wildcard ./src/libjsparser/*.c) \
 $(wildcard ./src/libecmaobjects/*.c) \
 $(wildcard ./src/libecmaoperations/*.c) \
 $(wildcard ./src/liballocator/*.c) \
 $(wildcard ./src/libcoreint/*.c) \
 $(wildcard ./src/liboptimizer/*.c) ) \
 $(wildcard src/libruntime/target/$(TARGET_SYSTEM)/*.c)

SOURCES_JERRY_H = \
 $(sort \
 $(wildcard ./src/libruntime/*.h) \
 $(wildcard ./src/libperipherals/*.h) \
 $(wildcard ./src/libjsparser/*.h) \
 $(wildcard ./src/libecmaobjects/*.h) \
 $(wildcard ./src/libecmaoperations/*.h) \
 $(wildcard ./src/liballocator/*.h) \
 $(wildcard ./src/libcoreint/*.h) \
 $(wildcard ./src/liboptimizer/*.h) ) \
 $(wildcard src/libruntime/target/$(TARGET_SYSTEM)/*.h)

SOURCES_JERRY_ASM = \
 $(wildcard src/libruntime/target/$(TARGET_SYSTEM)/*.S)

SOURCES_JERRY = $(SOURCES_JERRY_C) $(SOURCES_JERRY_ASM)

INCLUDES_JERRY = \
 -I src \
 -I src/libruntime \
 -I src/libperipherals \
 -I src/libjsparser \
 -I src/libecmaobjects \
 -I src/libecmaoperations \
 -I src/liballocator \
 -I src/liboptimizer \
 -I src/libcoreint

# libc
ifeq ($(OPTION_LIBC),musl)
  CC := musl-$(CC) 
  DEFINES_JERRY += -DLIBC_MUSL
  CFLAGS_COMMON += -static
else
  DEFINES_JERRY += -DLIBC_RAW
  CFLAGS_COMMON += -nostdlib

  ifeq ($(OPTION_SANITIZE),enable)
    CFLAGS_COMMON += -fsanitize=address
    LDFLAGS += -lasan
  endif
endif

ifeq ($(OPTION_NDEBUG),enable)
 DEFINES_JERRY += -DJERRY_NDEBUG
endif

ifeq ($(OPTION_MCU),disable)
 DEFINES_JERRY += -D__TARGET_HOST_x64 -DJERRY_SOURCE_BUFFER_SIZE=$$((1024*1024))
 CFLAGS_COMMON += -fno-stack-protector
else
 CFLAGS_COMMON += -ffunction-sections -fdata-sections -nostdlib
 DEFINES_JERRY += -D__TARGET_MCU
endif

ifeq ($(OPTION_COLOR),enable)
  CFLAGS_COMMON += -fdiagnostics-color=always
endif

ifeq ($(OPTION_TODO),enable)
 DEFINES_JERRY += -DJERRY_PRINT_TODO
endif

ifeq ($(OPTION_FIXME),enable)
 DEFINES_JERRY += -DJERRY_PRINT_FIXME
endif

ifeq ($(OPTION_VALGRIND),enable)
 VALGRIND_CMD := "valgrind --error-exitcode=254 --track-origins=yes"
else
 VALGRIND_CMD :=
 DEFINES_JERRY += -DJERRY_NVALGRIND
endif

#
# Third-party sources, headers, includes, cflags, ldflags
#

SOURCES_THIRDPARTY =
INCLUDES_THIRDPARTY = -I third-party/valgrind/
CFLAGS_THIRDPARTY =

ifeq ($(TARGET_SYSTEM),stm32f4)
 SOURCES_THIRDPARTY += \
 	 	./third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/CMSIS/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c \
		./third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/CMSIS/ST/STM32F4xx/Source/Templates/gcc_ride7/startup_stm32f4xx.s \
                ./third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_tim.c \
                ./third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_gpio.c \
                ./third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_rcc.c

 INCLUDES_THIRDPARTY += \
 	 	-I third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/CMSIS/ST/STM32F4xx/Include \
 	 	-I third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/STM32F4xx_StdPeriph_Driver/inc \
 	 	-I third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/CMSIS/Include \
 	 	-I third-party/STM32F4-Discovery_FW_V1.1.0/

#-I third-party/STM32F4-Discovery_FW_V1.1.0/Project/Demonstration \

endif

# Unit tests

SOURCES_UNITTESTS = \
 	 $(sort \
 	 $(patsubst %.c,%,$(notdir \
 	 $(wildcard $(UNITTESTS_SRC_DIR)/*.c))))

.PHONY: all clean check install $(JERRY_TARGETS) $(TESTS_TARGET)

all: clean $(JERRY_TARGETS)

$(JERRY_TARGETS):
	@rm -rf $(TARGET_DIR)
	@mkdir -p $(TARGET_DIR)
	@cppcheck $(DEFINES_JERRY) $(SOURCES_JERRY_C) $(INCLUDES_JERRY) $(INCLUDES_THIRDPARTY) \
          --error-exitcode=1 --std=c99 --enable=all --suppress=missingIncludeSystem --suppress=unusedFunction 1>/dev/null
	@vera++ -r ./tools/vera++ -p jerry $(SOURCES_JERRY_C) $(SOURCES_JERRY_H) --no-duplicate 1>$(TARGET_DIR)/vera.log
	@mkdir -p $(TARGET_DIR)/obj
	@source_index=0; \
	for jerry_src in $(SOURCES_JERRY) $(MAIN_MODULE_SRC); do \
		cmd="$(CC) -c $(DEFINES_JERRY) $(CFLAGS_COMMON) $(CFLAGS_JERRY) $(INCLUDES_JERRY) $(INCLUDES_THIRDPARTY) $$jerry_src \
                     -o $(TARGET_DIR)/obj/$$(basename $$jerry_src).$$source_index.o"; \
                if [ "$(OPTION_ECHO)" = "enable" ]; then echo $$cmd; echo; fi; \
		$$cmd; \
		if [ $$? -ne 0 ]; then echo Failed "'$$cmd'"; exit 1; fi; \
		source_index=$$(($$source_index+1)); \
	done; \
	for thirdparty_src in $(SOURCES_THIRDPARTY); do \
		cmd="$(CC) -c $(CFLAGS_COMMON) $(CFLAGS_THIRDPARTY) $(INCLUDES_THIRDPARTY) $$thirdparty_src \
                     -o $(TARGET_DIR)/obj/$$(basename $$thirdparty_src).$$source_index.o"; \
                if [ "$(OPTION_ECHO)" = "enable" ]; then echo $$cmd; echo; fi; \
		$$cmd; \
		if [ $$? -ne 0 ]; then echo Failed "'$$cmd'"; exit 1; fi; \
		source_index=$$(($$source_index+1)); \
	done; \
	cmd="$(CC) $(CFLAGS_COMMON) $(TARGET_DIR)/obj/* $(LDFLAGS) -o $(TARGET_DIR)/$(ENGINE_NAME)"; \
        if [ "$(OPTION_ECHO)" = "enable" ]; then echo $$cmd; echo; fi; \
	$$cmd; \
	if [ $$? -ne 0 ]; then echo Failed "'$$cmd'"; exit 1; fi;
	@if [ "$(OPTION_STRIP)" = "enable" ]; then $(STRIP) $(TARGET_DIR)/$(ENGINE_NAME) || exit $$?; fi;
	@if [ "$(OPTION_MCU)" = "enable" ]; then $(OBJCOPY) -Obinary $(TARGET_DIR)/$(ENGINE_NAME) $(TARGET_DIR)/$(ENGINE_NAME).bin || exit $$?; fi;
	@rm -rf $(TARGET_DIR)/obj

$(TESTS_TARGET):
	@rm -rf $(TARGET_DIR)
	@mkdir -p $(TARGET_DIR)
	@mkdir -p $(TARGET_DIR)/obj
	@cppcheck $(DEFINES_JERRY) `find $(UNITTESTS_SRC_DIR) -name *.[c]` $(SOURCES_JERRY_C) $(INCLUDES_JERRY) $(INCLUDES_THIRDPARTY) \
          --error-exitcode=1 --std=c99 --enable=all --suppress=missingIncludeSystem --suppress=unusedFunction 1>/dev/null
	@source_index=0; \
	for jerry_src in $(SOURCES_JERRY); \
        do \
                cmd="$(CC) -c $(DEFINES_JERRY) $(CFLAGS_COMMON) $(CFLAGS_JERRY) \
                $(INCLUDES_JERRY) $(INCLUDES_THIRDPARTY) $$jerry_src -o $(TARGET_DIR)/obj/$$(basename $$jerry_src).$$source_index.o"; \
                if [ "$(option_echo)" = "enable" ]; then echo $$cmd; echo; fi; \
                $$cmd; \
		if [ $$? -ne 0 ]; then echo Failed "'$$cmd'"; exit 1; fi; \
		source_index=$$(($$source_index+1)); \
        done
	@for unit_test in $(SOURCES_UNITTESTS); \
	do \
		cmd="$(CC) $(DEFINES_JERRY) $(CFLAGS_COMMON) $(CFLAGS_JERRY) \
		$(INCLUDES_JERRY) $(INCLUDES_THIRDPARTY) $(TARGET_DIR)/obj/*.o $(UNITTESTS_SRC_DIR)/$$unit_test.c -lc -o $(TARGET_DIR)/$$unit_test"; \
                if [ "$(OPTION_ECHO)" = "enable" ]; then echo $$cmd; echo; fi; \
		$$cmd; \
		if [ $$? -ne 0 ]; then echo Failed "'$$cmd'"; exit 1; fi; \
	done
	@ rm -rf $(TARGET_DIR)/obj
	@ VALGRIND=$(VALGRIND_CMD) ./tools/jerry_unittest.sh $(TARGET_DIR) $(TESTS_OPTS)

$(CHECK_TARGETS):
	@ if [ ! -f $(TARGET_DIR)/$(ENGINE_NAME) ]; then echo $(TARGET_OF_ACTION) is not built yet; exit 1; fi;
	@ if [ ! -d "$(TESTS_DIR)" ]; then echo \"$(TESTS_DIR)\" is not a directory; exit 1; fi;
	@ rm -rf $(TARGET_DIR)/check
	@ mkdir -p $(TARGET_DIR)/check
	@ if [ "$(OUTPUT_TO_LOG)" = "enable" ]; \
          then \
            ADD_OPTS="--output-to-log"; \
          fi; \
          VALGRIND=$(VALGRIND_CMD) ./tools/jerry_test.sh $(TARGET_DIR)/$(ENGINE_NAME) $(TARGET_DIR)/check $(TESTS_DIR) $(TESTS_OPTS) $$ADD_OPTS; \
          status_code=$$?; \
          if [ $$status_code -ne 0 ]; \
          then \
            echo $(TARGET) failed; \
            if [ "$(OUTPUT_TO_LOG)" = "enable" ]; \
            then \
              echo See log in $(TARGET_DIR)/check directory for details.; \
            fi; \
            \
            exit $$status_code; \
          fi;


$(FLASH_TARGETS): $(TARGET_OF_ACTION)
	st-flash write $(OUT_DIR)/$(TARGET_OF_ACTION)/jerry.bin 0x08000000 || exit $$?
