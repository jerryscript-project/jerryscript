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

include $(APPDIR)/Make.defs

# Jerryscript built-in application information.
PROGNAME  = jerry
PRIORITY  = $(CONFIG_INTERPRETERS_JERRYSCRIPT_PRIORITY)
STACKSIZE = $(CONFIG_INTERPRETERS_JERRYSCRIPT_STACKSIZE)

# Path to the JerryScript and NuttX projects. If not specified, it is
# supposed that JerryScript is located next to the nuttx folder.
JERRYSCRIPT_ROOT_DIR ?= $(abspath ../../../../jerryscript)
NUTTX_INCLUDE_DIR ?= $(abspath $(JERRYSCRIPT_ROOT_DIR)/../nuttx/include)

CFLAGS += -std=c99
CFLAGS += -I$(JERRYSCRIPT_ROOT_DIR)/jerry-core/include
CFLAGS += -I$(JERRYSCRIPT_ROOT_DIR)/jerry-ext/include
CFLAGS += -I$(JERRYSCRIPT_ROOT_DIR)/jerry-math/include

# These libs should be copied from the JerryScript project.
LIBS = libjerry-core.a libjerry-ext.a libjerry-math.a libjerry-port.a

ASRCS = setjmp.S
CSRCS = jerry-port.c
MAINSRC = jerry-main.c

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

.PHONY: libjerry
libjerry:
	$(JERRYSCRIPT_ROOT_DIR)/tools/build.py \
		--clean \
		--lto=OFF \
		--jerry-cmdline=OFF \
		--jerry-math=ON \
		--amalgam=ON \
		--mem-heap=70 \
		--profile=es.next \
		--compile-flag="-I${NUTTX_INCLUDE_DIR}" \
		--toolchain=$(abspath $(JERRYSCRIPT_ROOT_DIR)/cmake/toolchain_mcu_stm32f4.cmake)

clean:: cleanlibs

archive: libjerry $(LIBS) updateobjs

include $(APPDIR)/Application.mk
