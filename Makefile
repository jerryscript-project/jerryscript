# Copyright 2014 Samsung Electronics Co., Ltd.
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

TARGET ?= jerry
CROSS_COMPILE	?= arm-none-eabi-
OBJ_DIR = ./obj
OUT_DIR = ./out

MAIN_MODULE_SRC = ./src/main.c
UNITTESTS_SRC_DIR = ./tests/unit

LNK_SCRIPT_STM32F4 = ./third-party/stm32f4.ld
SUP_STM32F4 = ./third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/CMSIS/ST/STM32F4xx/Source/Templates/gcc_ride7/startup_stm32f4xx.s


# FIXME:
#  Place jerry-libc.c, pretty-printer.c to some subdirectory (libruntime?)
#  and add them to the SOURCES list through wildcard.
# FIXME:
#  Add common-io.c and sensors.c
SOURCES = \
	$(sort \
	$(wildcard ./src/libruntime/*.c) \
	$(wildcard ./src/libperipherals/actuators.c) \
	$(wildcard ./src/libjsparser/*.c) \
	$(wildcard ./src/libecmaobjects/*.c) \
	$(wildcard ./src/libecmaoperations/*.c) \
	$(wildcard ./src/liballocator/*.c) \
	$(wildcard ./src/libcoreint/*.c) )

SOURCES_STM32F4 = \
	third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/CMSIS/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c \
        $(wildcard src/libruntime/stm32f4/*)

SOURCES_LINUX = \
        $(wildcard src/libruntime/linux/*)

HEADERS = \
	$(sort \
	$(wildcard ./src/*.h) \
	$(wildcard ./src/libruntime/*.h) \
	$(wildcard ./src/libperipherals/*.h) \
	$(wildcard ./src/libjsparser/*.h) \
	$(wildcard ./src/libecmaobjects/*.h) \
	$(wildcard ./src/libecmaoperations/*.h) \
	$(wildcard ./src/liballocator/*.h) \
	$(wildcard ./src/libcoreint/*.h) )

INCLUDES = \
	-I src \
	-I src/libruntime \
	-I src/libperipherals \
	-I src/libjsparser \
	-I src/libecmaobjects \
	-I src/libecmaoperations \
	-I src/liballocator \
	-I src/libcoreint

INCLUDES_STM32F4 = \
	-I third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/CMSIS/ST/STM32F4xx/Include \
	-I third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/STM32F4xx_StdPeriph_Driver/inc \
	-I third-party/STM32F4-Discovery_FW_V1.1.0/Libraries/CMSIS/Include

UNITTESTS = \
	$(sort \
	$(patsubst %.c,%,$(notdir \
	$(wildcard $(UNITTESTS_SRC_DIR)/*))))

OBJS = \
	$(sort \
	$(patsubst %.c,./%.o,$(notdir $(MAIN_MODULE_SRC) $(SOURCES))))

CC  = gcc
LD  = ld
OBJDUMP	= objdump
OBJCOPY	= objcopy
SIZE	= size
STRIP	= strip

# General flags
CFLAGS ?= $(INCLUDES) -std=c99 #-fdiagnostics-color=always
CFLAGS += -Wall -Wextra -Wpedantic -Wlogical-op -Winline
CFLAGS += -Wformat-nonliteral -Winit-self -Wstack-protector
CFLAGS += -Wconversion -Wsign-conversion -Wformat-security
CFLAGS += -Wstrict-prototypes -Wmissing-prototypes

# Flags for MCU
MCU_CFLAGS += -mlittle-endian -mcpu=cortex-m4  -march=armv7e-m -mthumb
MCU_CFLAGS += -mfpu=fpv4-sp-d16 -mfloat-abi=hard
MCU_CFLAGS += -ffunction-sections -fdata-sections -nostdlib -fno-common 

LDFLAGS = -nostartfiles -T$(LNK_SCRIPT_STM32F4)

DEBUG_OPTIONS = -g3 -O0 # -fsanitize=address
RELEASE_OPTIONS = -Os -Werror -DJERRY_NDEBUG

DEFINES = -DMEM_HEAP_CHUNK_SIZE=256 -DMEM_HEAP_AREA_SIZE=32768 -DMEM_STATS
TARGET_HOST = -D__HOST
TARGET_MCU = -D__TARGET_MCU

#-I third-party/STM32F4-Discovery_FW_V1.1.0/Project/Demonstration \

.PHONY: all debug debug.stdm32f4 release clean tests check install

all: clean debug release check

debug.stdm32f4: clean debug.stdm32f4.bin

debug.stdm32f4.o:
	mkdir -p $(OUT_DIR)/debug.stdm32f4/
	$(CROSS_COMPILE)$(CC) \
	$(SUP_STM32F4) $(SOURCES_STM32F4) $(INCLUDES_STM32F4) \
	$(CFLAGS) $(MCU_CFLAGS) $(DEBUG_OPTIONS) \
	$(DEFINES) $(TARGET_MCU) $(MAIN_MODULE_SRC) -c

debug.stdm32f4.elf: debug.stdm32f4.o
	$(CROSS_COMPILE)$(LD) $(LDFLAGS) -o $(TARGET).elf *.o
	rm -f *.o
 
debug.stdm32f4.bin: debug.stdm32f4.elf
	$(CROSS_COMPILE)$(OBJCOPY) -Obinary $(TARGET).elf $(TARGET).bin
	rm -f *.elf

debug: clean
	mkdir -p $(OUT_DIR)/debug.host/
	$(CC) $(CFLAGS) $(DEBUG_OPTIONS) $(DEFINES) $(TARGET_HOST) \
	$(SOURCES) $(SOURCES_LINUX) $(MAIN_MODULE_SRC) -o $(OUT_DIR)/debug.host/$(TARGET)

release: clean
	mkdir -p $(OUT_DIR)/release.host/
	$(CC) $(CFLAGS) $(RELEASE_OPTIONS) $(DEFINES) $(TARGET_HOST) \
	$(SOURCES) $(SOURCES_LINUX) $(MAIN_MODULE_SRC) -o $(OUT_DIR)/release.host/$(TARGET)
	$(STRIP) $(OUT_DIR)/release.host/$(TARGET)

tests:
	mkdir -p $(OUT_DIR)/tests.host/
	for unit_test in $(UNITTESTS); \
	do \
		$(CC) -O3 $(CFLAGS) $(DEBUG_OPTIONS) $(DEFINES) $(TARGET_HOST) \
		$(SOURCES) $(SOURCES_LINUX) $(UNITTESTS_SRC_DIR)/"$$unit_test".c -o $(OUT_DIR)/tests.host/"$$unit_test"; \
	done

clean:
	rm -f $(OBJ_DIR)/*.o *.bin *.o *~ *.log *.log
	rm -rf $(OUT_DIR)
	rm -f $(TARGET)
	rm -f $(TARGET).elf
	rm -f $(TARGET).bin
	rm -f $(TARGET).map
	rm -f $(TARGET).hex
	rm -f $(TARGET).lst
	rm -f js.files

check: tests
	@mkdir -p $(OUT_DIR)
	@cd $(OUT_DIR)

	@echo "=== Running cppcheck ==="
	@cppcheck $(HEADERS) $(SOURCES) --enable=all --std=c99
	@echo Done
	@echo
	
	@echo "=== Running unit tests ==="
	./tools/jerry_unittest.sh $(OUT_DIR)/tests.host $(UNITTESTS)
	@echo Done
	@echo
	
	@echo "=== Running js tests ==="
	@ if [ -f $(OUT_DIR)/release.host/$(TARGET) ]; then \
		./tools/jerry_test.sh $(OUT_DIR)/release.host/$(TARGET);\
	fi
	
	@if [ -f $(OUT_DIR)/debug.host/$(TARGET) ]; then \
		./tools/jerry_test.sh $(OUT_DIR)/debug.host/$(TARGET); \
	fi
	@echo Done
	@echo

install:
	st-flash write $(TARGET).bin 0x08000000
