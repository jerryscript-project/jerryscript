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
CROSS_COMPILE	?= 
OBJ_DIR = obj
OUT_DIR = ./out

MAIN_MODULE_SRC = ./src/main.c
UNITTESTS_SRC_DIR = ./tests/unit

# FIXME:
#  Place jerry-libc.c, pretty-printer.c to some subdirectory (libruntime?)
#  and add them to the SOURCES list through wildcard.
# FIXME:
#  Add common-io.c and sensors.c
SOURCES = \
	$(sort \
	$(wildcard ./src/jerry-libc.c ./src/pretty-printer.c) \
	$(wildcard ./src/libperipherals/actuators.c) \
	$(wildcard ./src/libjsparser/*.c) \
	$(wildcard ./src/libecmaobjects/*.c) \
	$(wildcard ./src/liballocator/*.c) \
	$(wildcard ./src/libcoreint/*.c) )

HEADERS = \
	$(sort \
	$(wildcard ./src/*.h) \
	$(wildcard ./src/libperipherals/*.h) \
	$(wildcard ./src/libjsparser/*.h) \
	$(wildcard ./src/libecmaobjects/*.h) \
	$(wildcard ./src/liballocator/*.h) \
	$(wildcard ./src/libcoreint/*.h) )

INCLUDES = \
	-I src \
	-I src/libperipherals \
	-I src/libjsparser \
	-I src/libecmaobjects \
	-I src/liballocator \
	-I src/libcoreint

UNITTESTS = \
	$(sort \
	$(patsubst %.c,%,$(notdir \
	$(wildcard $(UNITTESTS_SRC_DIR)/*))))

OBJS = \
	$(sort \
	$(patsubst %.c,./$(OBJ_DIR)/%.o,$(notdir $(MAIN_MODULE_SRC) $(SOURCES))))

CC  = $(CROSS_COMPILE)gcc
LD  = $(CROSS_COMPILE)ld
OBJDUMP	= $(CROSS_COMPILE)objdump
OBJCOPY	= $(CROSS_COMPILE)objcopy
SIZE	= $(CROSS_COMPILE)size
STRIP	= $(CROSS_COMPILE)strip

# General flags
CFLAGS ?= $(INCLUDES) -std=c99 -m32 #-fdiagnostics-color=always
#CFLAGS += -Wall -Wextra -Wpedantic -Wlogical-op -Winline
#CFLAGS += -Wformat-nonliteral -Winit-self -Wstack-protector
#CFLAGS += -Wconversion -Wsign-conversion -Wformat-security
#CFLAGS += -Wstrict-prototypes -Wmissing-prototypes

# Flags for MCU
MCU_CFLAGS += -mlittle-endian -mcpu=cortex-m4  -march=armv7e-m -mthumb
MCU_CFLAGS += -mfpu=fpv4-sp-d16 -mfloat-abi=hard
MCU_CFLAGS += -ffunction-sections -fdata-sections

DEBUG_OPTIONS = -g3 -O0 -DJERRY_NDEBUG# -fsanitize=address
RELEASE_OPTIONS = -Os -Werror -DJERRY_NDEBUG

DEFINES = -DMEM_HEAP_CHUNK_SIZE=256 -DMEM_HEAP_AREA_SIZE=32768 -DMEM_STATS
TARGET_HOST = -D__HOST
TARGET_MCU = -D__MCU

.PHONY: all debug debug.stm32f3 release clean tests check install

all: clean debug release check

debug: clean
	mkdir -p $(OUT_DIR)/debug.host/
	$(CC) $(CFLAGS) $(DEBUG_OPTIONS) $(DEFINES) $(TARGET_HOST) \
	$(SOURCES) $(MAIN_MODULE_SRC) -o $(OUT_DIR)/debug.host/$(TARGET)

release: clean
	mkdir -p $(OUT_DIR)/release.host/
	$(CC) $(CFLAGS) $(RELEASE_OPTIONS) $(DEFINES) $(TARGET_HOST) \
	$(SOURCES) $(MAIN_MODULE_SRC) -o $(OUT_DIR)/release.host/$(TARGET)
	$(STRIP) $(OUT_DIR)/release.host/$(TARGET)

tests:
	mkdir -p $(OUT_DIR)/tests.host/
	for unit_test in $(UNITTESTS); \
	do \
		$(CC) -O3 $(CFLAGS) $(DEBUG_OPTIONS) $(DEFINES) $(TARGET_HOST) \
		$(SOURCES) $(UNITTESTS_SRC_DIR)/"$$unit_test".c -o $(OUT_DIR)/tests.host/"$$unit_test"; \
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
