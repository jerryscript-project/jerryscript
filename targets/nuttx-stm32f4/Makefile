# Copyright JS Foundation and other contributors, http://js.foundation
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

-include $(TOPDIR)/Make.defs

# Jerryscript built-in application info

CONFIG_JERRYSCRIPT_PRIORITY ?= SCHED_PRIORITY_DEFAULT
CONFIG_JERRYSCRIPT_STACKSIZE ?= 16384
CONFIG_JERRYSCRIPT_HEAPSIZE ?= 107520

APPNAME = jerry
# path to the project dir, "jerry-nuttx" by default
ROOT_DIR = ../../..
PRIORITY = $(CONFIG_JERRYSCRIPT_PRIORITY)
STACKSIZE = $(CONFIG_JERRYSCRIPT_STACKSIZE)
CFLAGS += -std=c99 -DJERRY_NDEBUG '-DCONFIG_MEM_HEAP_AREA_SIZE=$(CONFIG_JERRYSCRIPT_HEAPSIZE)'
CFLAGS += -I$(ROOT_DIR)/ $(shell find $(ROOT_DIR)/jerryscript/jerry-core -type d | sed -r -e 's/^/-I/g')

# Jerryscript

.PHONY: jerry_core_allin.c
jerry_core_allin.c:
	echo '#include "jerryscript/jerry-libm/nextafter.c"' > jerry_core_allin.c
	find $(ROOT_DIR)/jerryscript/jerry-core -name "*.c" | sed -r -e 's/(\.\.\/)*(.+)/#include "\2"/g' >> jerry_core_allin.c

ASRCS =
CSRCS = jerry_core_allin.c
MAINSRC = jerry_main.c

CONFIG_JERRYSCRIPT_PROGNAME ?= jerry$(EXEEXT)
PROGNAME = $(CONFIG_JERRYSCRIPT_PROGNAME)

include $(APPDIR)/Application.mk
