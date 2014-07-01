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

OBJ_DIR = obj

SOURCES = \
	$(sort \
	$(wildcard ./src/*.c))

INCLUDES = -I src

OBJS = $(sort \
	$(patsubst %.c,./$(OBJ_DIR)/%.o,$(notdir $(SOURCES))))

#CROSS_COMPILE	?= arm-none-eabi-

CC  = $(CROSS_COMPILE)gcc
LD  = $(CROSS_COMPILE)ld
OBJDUMP	= $(CROSS_COMPILE)objdump
OBJCOPY	= $(CROSS_COMPILE)objcopy
SIZE	= $(CROSS_COMPILE)size

# General flags
CFLAGS ?= $(INCLUDES) -Wall -std=c99 -fdiagnostics-color=always
CFLAGS += -Wextra -Wpedantic -Wformat-security -Wlogical-op
CFLAGS += -Wformat-nonliteral -Winit-self -Wstack-protector
CFLAGS += -Wconversion -Wsign-conversion -Winline
CFLAGS += -Wstrict-prototypes -Wmissing-prototypes

# Flags for MCU
#CFLAGS += -mlittle-endian -mcpu=cortex-m4  -march=armv7e-m -mthumb
#CFLAGS += -mfpu=fpv4-sp-d16 -mfloat-abi=hard
#CFLAGS += -ffunction-sections -fdata-sections

DEBUG_OPTIONS = -fsanitize=address -g3 -O0 -DDEBUG
RELEASE_OPTIONS = -Os -Werror

.PHONY: all debug release clean install test

all: debug

debug:
	$(CC) $(INCLUDES) $(CFLAGS) $(DEBUG_OPTIONS) $(SOURCES) \
	-o $(TARGET)

release:
	$(CC) $(INCLUDES) $(CFLAGS) $(RELEASE_OPTIONS) $(SOURCES) \
	-o $(TARGET)

clean:
	rm -f $(OBJ_DIR)/*.o *.o
	rm -f $(TARGET)
	rm -f $(TARGET).elf
	rm -f $(TARGET).bin
	rm -f $(TARGET).map
	rm -f $(TARGET).hex
	rm -f $(TARGET).lst

install:
	st-flash write $(TARGET).bin 0x08000000

test:
	./tools/jerry_test.sh
