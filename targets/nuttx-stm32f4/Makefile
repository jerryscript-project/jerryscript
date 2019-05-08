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

# Jerryscript built-in application information.
CONFIG_JERRYSCRIPT_PRIORITY ?= SCHED_PRIORITY_DEFAULT
CONFIG_JERRYSCRIPT_PROGNAME ?= jerry$(EXEEXT)
CONFIG_JERRYSCRIPT_STACKSIZE ?= 16384

PROGNAME = $(CONFIG_JERRYSCRIPT_PROGNAME)
PRIORITY = $(CONFIG_JERRYSCRIPT_PRIORITY)
STACKSIZE = $(CONFIG_JERRYSCRIPT_STACKSIZE)

# Path to the JerryScript project. If not specified, it is supposed
# that JerryScript is located next to the nuttx-apps folder.
JERRYSCRIPT_ROOT_DIR ?= ../../../jerryscript

CFLAGS += -std=c99
CFLAGS += -I$(JERRYSCRIPT_ROOT_DIR)/jerry-core/include
CFLAGS += -I$(JERRYSCRIPT_ROOT_DIR)/jerry-ext/include
CFLAGS += -I$(JERRYSCRIPT_ROOT_DIR)/jerry-libm/include

# These libs should be copied from the JerryScript project.
LIBS = libjerry-core.a libjerry-ext.a libjerry-libm.a

APPNAME = jerry
ASRCS = setjmp.S
CSRCS = jerry_port.c
MAINSRC = jerry_main.c

.PHONY: copylibs
copylibs:
	cp $(JERRYSCRIPT_ROOT_DIR)/build/lib/lib*.a .

$(LIBS): copylibs
	$(firstword $(AR)) x $@

.PHONY: updateobjs
updateobjs:
	$(eval OBJS += $(shell find . -name "*.obj"))

.PHONY: cleanlibs
cleanlibs: updateobjs
	rm -f $(OBJS)

clean:: cleanlibs

.built: $(LIBS) updateobjs

include $(APPDIR)/Application.mk
