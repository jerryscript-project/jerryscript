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

TARGET	?= jerry

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

CFLAGS ?= -Wall -std=c99 -Wextra -Wpedantic -fdiagnostics-color=always
#CFLAGS += -Werror
#CFLAGS += -Wformat-security -Wformat-nonliteral -Winit-self
#CFLAGS += -Wconversion -Wsign-conversion -Wlogical-op
#CFLAGS += -Wstrict-prototypes -Wmissing-prototypes
#CFLAGS += -Winline -Wstack-protector

#CFLAGS += -mlittle-endian -mcpu=cortex-m4  -march=armv7e-m -mthumb
#CFLAGS += -mfpu=fpv4-sp-d16 -mfloat-abi=hard
#CFLAGS += -ffunction-sections -fdata-sections

#DEBUG_OPTIONS = -fsanitize=address -g3 -O0
#RELEASE_OPTIONS = -Os

HEADERS = error.h lexer.h pretty-printer.h parser.h
#OBJS = lexer.o pretty-printer.o parser.o main.o
DEFINES = -DDEBUG

all:
	$(CC) $(INCLUDES) $(CFLAGS) $(DEBUG_OPTIONS) $(DEFINES) $(SOURCES) \
	-o $(TARGET)

clean:
	rm -f $(OBJ_DIR)/*.o *.o $(TARGET)

test:
	./tools/jerry_test.sh
