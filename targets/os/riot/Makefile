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

# application name
APPLICATION = riot_jerryscript

# default BOARD enviroment
BOARD ?= stm32f4discovery

# path to jerryscript/targets/os/riot
TARGET_DIR = $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

# path to the JERRYSCRIPT directory
JERRYDIR ?= $(TARGET_DIR)/../../..

# path to the RIOT base directory
RIOTBASE ?= $(JERRYDIR)/../RIOT

# path to the application directory
APPDIR ?= $(JERRYDIR)/targets/os/riot/source

# path to the binary directory
BUILDDIR ?= $(JERRYDIR)/build/riot-stm32f4

# path to the binary directory
BINDIR ?= $(BUILDDIR)/bin

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

INCLUDES += -I$(JERRYDIR)/jerry-core/include
INCLUDES += -I$(JERRYDIR)/jerry-ext/include

ARCHIVES += $(JERRYDIR)/build/lib/libjerry-core.a
ARCHIVES += $(JERRYDIR)/build/lib/libjerry-ext.a
ARCHIVES += $(JERRYDIR)/build/lib/libjerry-port.a

USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += posix_sleep

.PHONY: libjerry
libjerry:
	$(JERRYDIR)/tools/build.py \
		--clean \
		--lto=OFF \
		--jerry-cmdline=OFF \
		--jerry-math=ON \
		--amalgam=ON \
		--mem-heap=70 \
		--profile=es.next \
		--compile-flag=-fshort-enums \
		--toolchain=$(abspath $(JERRYDIR)/cmake/toolchain_mcu_stm32f4.cmake)

.PHONY: clear-cflags
clear-cflags:
	$(eval BACKUP := $(CFLAGS))
	$(eval CFLAGS := )

.PHONY: restore-cflags
restore-cflags:
	$(eval CFLAGS := $(BACKUP))

# CFLAGS is filled by Makefile.include that is only for RIOT.
# Clear CFLAGS temporarily while compiling JerryScript.
all: clear-cflags libjerry restore-cflags

include $(RIOTBASE)/Makefile.include
