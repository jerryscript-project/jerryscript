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

# use TAB-8
.DEFAULT_GOAL := all

ifndef ZEPHYR_BASE
$(error Missing zephyr base)
endif

# For testing without real hardware use qemu_x86 instead of arduino_101
BOARD ?= arduino_101

O ?= $(PROJECT_BASE)/outdir

ZEPHYR    ?= $(ZEPHYR_BASE)
TYPE      ?= release
JERRYHEAP ?= 16

ZEPHYRINC   = $(ZEPHYR_BASE)/include
ZEPHYRLIB   = $(ZEPHYR_BASE)/lib

TARGET_ZEPHYR ?= ./targets/zephyr
SOURCE_DIR = $(TARGET_ZEPHYR)/src

export JERRY_INCLUDE = -I$(CURDIR)/jerry-core/include -I$(CURDIR)/jerry-ext/include

MDEF_FILE = $(realpath $(SOURCE_DIR)/../prj.mdef)
CONF_FILE = $(realpath $(SOURCE_DIR)/../prj.conf)

ifdef V
$(info Zephyr)
$(info SOURCE_DIR=$(SOURCE_DIR))
$(info JERRY_INCLUDE=$(JERRY_INCLUDE))
$(info CONF_FILE =$(CONF_FILE))
endif

KERNEL_TYPE = micro

KBUILD_VERBOSE = $(V)

APP = main-zephyr.c

ALL_LIBS += $(USER_LIBS)
export ALL_LIBS
LDFLAGS_zephyr += $(USER_LIB_INCLUDE_DIR)
export LDFLAGS_zephyr

include ${ZEPHYR_BASE}/Makefile.inc

.PHONY = showconfig
