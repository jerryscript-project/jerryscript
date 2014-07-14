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
#   Main targets: {dev,debug,release,debug_release}.{linux,stm32f{4}}[.{check,flash}]
#
#    Target mode part (before dot):
#       dev:           - JERRY_NDEBUG; - optimizations; + debug symbols; - -Werror  | local development build
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

export TARGET_MODES = dev debug debug_release release
export TARGET_PC_SYSTEMS = linux
export TARGET_MCU_SYSTEMS = $(addprefix stm32f,4) # now only stm32f4 is supported, to add, for example, to stm32f3, change to $(addprefix stm32f,3 4)
export TARGET_SYSTEMS = $(TARGET_PC_SYSTEMS) $(TARGET_MCU_SYSTEMS)

# Target list
export JERRY_TARGETS = $(foreach __MODE,$(TARGET_MODES),$(foreach __SYSTEM,$(TARGET_SYSTEMS),$(__MODE).$(__SYSTEM)))
export TESTS_TARGET = unittests
export CHECK_TARGETS = $(foreach __TARGET,$(JERRY_TARGETS),$(__TARGET).check)
export FLASH_TARGETS = $(foreach __TARGET,$(foreach __MODE,$(TARGET_MODES),$(foreach __SYSTEM,$(TARGET_MCU_SYSTEMS),$(__MODE).$(__SYSTEM))),$(__TARGET).flash)

export OBJ_DIR = ./obj
export OUT_DIR = ./out
export UNITTESTS_SRC_DIR = ./tests/unit

all: clean $(JERRY_TARGETS) $(TESTS_TARGET) $(CHECK_TARGETS)

$(JERRY_TARGETS) $(TESTS_TARGET) $(FLASH_TARGETS) $(CHECK_TARGETS):
	@echo $@
	@make -f Makefile.mak TARGET=$@ $@

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
