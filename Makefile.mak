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
TARGET_SPACED = $(subst ., ,$(TARGET))
#   extract target mode part
TARGET_MODE   = $(word 1,$(TARGET_SPACED))
#   extract target system part
TARGET_SYSTEM = $(word 2,$(TARGET_SPACED))
#   extract optional action part
TARGET_ACTION = $(word 3,$(TARGET_SPACED))

# Target used as dependency of an action (check, flash, etc.)
TARGET_OF_ACTION = $(TARGET_MODE).$(TARGET_SYSTEM)

# target folder name in $(OUT_DIR)
TARGET_DIR=$(OUT_DIR)/$(TARGET_MODE).$(TARGET_SYSTEM)

# unittests mode -> linux system
ifeq ($(TARGET_MODE),$(TESTS_TARGET))
 TARGET_SYSTEM = linux
endif

#
# Options setup
#

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

# -Werror
ifeq ($(TARGET_MODE),dev)
 OPTION_WERROR = disable
else
 OPTION_WERROR = enable
endif

# Is MCU target?
ifeq ($(filter-out $(TARGET_MCU_SYSTEMS),$(TARGET_SYSTEM)),)
	OPTION_MCU = enable
else
	OPTION_MCU = disable
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

# Optimizations
CFLAGS_OPTIMIZE ?= -Os -flto
CFLAGS_NO_OPTIMIZE ?= -O0
LDFLAGS_OPTIMIZE ?=
LDFLAGS_NO_OPTIMIZE ?=

# Debug symbols
CFLAGS_DEBUG_SYMS ?= -g3

# Cortex-M4 MCU
CFLAGS_CORTEXM4 ?= -mlittle-endian -mcpu=cortex-m4 -march=armv7e-m -mthumb \
		   -mfpu=fpv4-sp-d16 -mfloat-abi=hard 


#
# Common
#

CFLAGS_COMMON ?= $(INCLUDES) -std=c99 # -fsanitize=address -fdiagnostics-color=always 

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

CFLAGS_JERRY = $(CFLAGS_WARNINGS)
DEFINES_JERRY = -DMEM_HEAP_CHUNK_SIZE=256 -DMEM_HEAP_AREA_SIZE=32768 -DMEM_STATS

# FIXME:
#  Add common-io.c and sensors.c
SOURCES_JERRY = \
 $(sort \
 $(wildcard ./src/libruntime/*.c) \
 $(wildcard ./src/libperipherals/actuators.c) \
 $(wildcard ./src/libjsparser/*.c) \
 $(wildcard ./src/libecmaobjects/*.c) \
 $(wildcard ./src/libecmaoperations/*.c) \
 $(wildcard ./src/liballocator/*.c) \
 $(wildcard ./src/libcoreint/*.c) ) \
 $(wildcard src/libruntime/target/$(TARGET_SYSTEM)/*)

INCLUDES_JERRY = \
 -I src \
 -I src/libruntime \
 -I src/libperipherals \
 -I src/libjsparser \
 -I src/libecmaobjects \
 -I src/libecmaoperations \
 -I src/liballocator \
 -I src/libcoreint

ifeq ($(OPTION_NDEBUG),enable)
 DEFINES_JERRY += -DJERRY_NDEBUG
endif

ifeq ($(OPTION_WERROR),enable)
 CFLAGS_JERRY += $(CFLAGS_WERROR)
endif

ifeq ($(OPTION_MCU),disable)
 DEFINES_JERRY += -D__HOST
else
 CFLAGS_COMMON += -ffunction-sections -fdata-sections -nostdlib
 DEFINES_JERRY += -D__TARGET_MCU
endif

#
# Third-party sources, headers, includes, cflags, ldflags
#

SOURCES_THIRDPARTY =
INCLUDES_THIRDPARTY =
CFLAGS_THIRDPARTY =

ifeq ($(TARGET_SYSTEM),stm32f4)
 SOURCES_THIRDPARTY += \
 	 	./third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/CMSIS/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c \
		./third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/CMSIS/ST/STM32F4xx/Source/Templates/gcc_ride7/startup_stm32f4xx.s

 INCLUDES_THIRDPARTY += \
 	 	-I third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/CMSIS/ST/STM32F4xx/Include \
 	 	-I third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/STM32F4xx_StdPeriph_Driver/inc \
 	 	-I third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/CMSIS/Include

#-I third-party/STM32F4-Discovery_FW_V1.1.0/Project/Demonstration \

endif

# Unit tests

SOURCES_UNITTESTS = \
 	 $(sort \
 	 $(patsubst %.c,%,$(notdir \
 	 $(wildcard $(UNITTESTS_SRC_DIR)/*))))

.PHONY: all clean check install $(JERRY_TARGETS) $(TESTS_TARGET)

all: clean $(JERRY_TARGETS)

$(JERRY_TARGETS):
	@rm -rf $(TARGET_DIR)
	@mkdir -p $(TARGET_DIR)
	@rm -rf $(OBJ_DIR)
	@mkdir $(OBJ_DIR)
	@source_index=0; \
	for jerry_src in $(SOURCES_JERRY) $(MAIN_MODULE_SRC); do \
		cmd="$(CC) -c $(DEFINES_JERRY) $(CFLAGS_COMMON) $(CFLAGS_JERRY) $(INCLUDES_JERRY) $(INCLUDES_THIRDPARTY) $$jerry_src -o $(OBJ_DIR)/$$(basename $$jerry_src).$$source_index.o"; \
		$$cmd; \
		if [ $$? -ne 0 ]; then echo Failed "'$$cmd'"; exit 1; fi; \
		source_index=$$(($$source_index+1)); \
	done; \
	for thirdparty_src in $(SOURCES_THIRDPARTY); do \
		cmd="$(CC) -c $(CFLAGS_COMMON) $(CFLAGS_THIRDPARTY) $(INCLUDES_THIRDPARTY) $$thirdparty_src -o $(OBJ_DIR)/$$(basename $$thirdparty_src).$$source_index.o"; \
		$$cmd; \
		if [ $$? -ne 0 ]; then echo Failed "'$$cmd'"; exit 1; fi; \
		source_index=$$(($$source_index+1)); \
	done; \
	cmd="$(CC) $(CFLAGS_COMMON) $(OBJ_DIR)/* $(LDFLAGS) -o $(TARGET_DIR)/$(ENGINE_NAME)"; \
	$$cmd; \
	if [ $$? -ne 0 ]; then echo Failed "'$$cmd'"; exit 1; fi;
	@if [ "$(OPTION_STRIP)" = "enable" ]; then $(STRIP) $(TARGET_DIR)/$(ENGINE_NAME) || exit $$?; fi;
	@if [ "$(OPTION_MCU)" = "enable" ]; then $(OBJCOPY) -Obinary $(TARGET_DIR)/$(ENGINE_NAME) $(TARGET_DIR)/$(ENGINE_NAME).bin || exit $$?; fi;

$(TESTS_TARGET):
	@echo $@ $(TARGET_DIR)
	@mkdir -p $(TARGET_DIR)
	@for unit_test in $(SOURCES_UNITTESTS); \
	do \
		$(CC) $(DEFINES_JERRY) $(CFLAGS_COMMON) $(CFLAGS_JERRY) \
		$(INCLUDES_JERRY) $(INCLUDES_THIRDPARTY) $(SOURCES_JERRY) $(UNITTESTS_SRC_DIR)/"$$unit_test".c -o $(TARGET_DIR)/"$$unit_test"; \
	done
	@ echo "=== Running unit tests ==="
	@ ./tools/jerry_unittest.sh $(TARGET_DIR)
	@ echo Done
	@ echo

# FIXME: Change cppcheck's --error-exitcode to 1 after fixing cppcheck's warnings and errors.
$(CHECK_TARGETS): $(TARGET_OF_ACTION)
	@ echo "=== Running cppcheck ==="
	@ cppcheck $(DEFINES_JERRY) `find $(UNITTESTS_SRC_DIR) -name *.[c]` $(SOURCES_JERRY) $(INCLUDES_JERRY) $(INCLUDES_THIRDPARTY) --error-exitcode=0 --enable=all --std=c99
	@ echo Done
	@ echo
	
	@ echo "=== Running js tests ==="
	@ if [ -f $(TARGET_DIR)/$(ENGINE_NAME) ]; then \
		./tools/jerry_test.sh $(TARGET_DIR)/$(ENGINE_NAME); \
	fi
	
	@echo Done
	@echo

$(FLASH_TARGETS): $(TARGET_OF_ACTION)
	st-flash write $(OUT_DIR)/$(TARGET_OF_ACTION)/jerry.bin 0x08000000 || exit $$?
