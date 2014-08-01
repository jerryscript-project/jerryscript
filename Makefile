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

#
# Target naming scheme
#
#   Main targets: {debug,release,debug_release}.{linux,stm32f{4}}[.{check,flash}]
#
#    Target mode part (before dot):
#       debug:         - JERRY_NDEBUG; - optimizations; + debug symbols; + -Werror  | debug build
#       debug_release: - JERRY_NDEBUG; + optimizations; + debug symbols; + -Werror  | checked release build
#       release:       + JERRY_NDEBUG; + optimizations; - debug symbols; + -Werror  | release build
#
#    Target system part (after first dot):
#       linux - target system is linux
#       stm32f{4} - target is STM32F{4} board 
#
#    Target action part (optional, after second dot):
#       check - run cppcheck on src folder, unit and other tests
#       flash - flash specified mcu target binary
#
#
#   Unit test target: unittests
#
# Options
#
#   dwarf4=1 - use DWARF v4 format for debug information
#

export TARGET_MODES = debug debug_release release
export TARGET_PC_SYSTEMS = linux
export TARGET_MCU_SYSTEMS = $(addprefix stm32f,4) # now only stm32f4 is supported, to add, for example, to stm32f3, change to $(addprefix stm32f,3 4)
export TARGET_SYSTEMS = $(TARGET_PC_SYSTEMS) $(TARGET_MCU_SYSTEMS)

# Target list
export JERRY_TARGETS = $(foreach __MODE,$(TARGET_MODES),$(foreach __SYSTEM,$(TARGET_SYSTEMS),$(__MODE).$(__SYSTEM)))
export TESTS_TARGET = unittests
export PARSER_TESTS_TARGET = testparser
export CHECK_TARGETS = $(foreach __TARGET,$(JERRY_TARGETS),$(__TARGET).check)
export FLASH_TARGETS = $(foreach __TARGET,$(foreach __MODE,$(TARGET_MODES),$(foreach __SYSTEM,$(TARGET_MCU_SYSTEMS),$(__MODE).$(__SYSTEM))),$(__TARGET).flash)

export OUT_DIR = ./out
export UNITTESTS_SRC_DIR = ./tests/unit

export SHELL=/bin/bash

export dwarf4
export echo
export todo
export fixme
export color
export sanitize
export valgrind
export musl
export libc_raw

all: clean $(TESTS_TARGET) $(CHECK_TARGETS)

$(JERRY_TARGETS) $(TESTS_TARGET) $(PARSER_TESTS_TARGET) $(FLASH_TARGETS) $(CHECK_TARGETS):
	@echo $@
	@$(MAKE) -f Makefile.mk TARGET=$@ $@

clean:
	rm -rf $(OUT_DIR)
