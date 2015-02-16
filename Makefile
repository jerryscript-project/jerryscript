# Copyright 2014-2015 Samsung Electronics Co., Ltd.
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
#   Main targets: {debug,release}.{linux,stm32f{4}}[.{check,flash}]
#
#    Target mode part (before dot):
#       debug:         - JERRY_NDEBUG; - optimizations; + debug symbols; + -Werror  | debug build
#       release:       + JERRY_NDEBUG; + optimizations; - debug symbols; + -Werror  | release build
#
#    Target system and modifiers part (after first dot):
#       linux - target system is linux
#       stm32f{4} - target is STM32F{4} board
#
#       Modifiers can be added after '-' sign.
#        For list of modifiers for PC target - see TARGET_PC_MODS, for MCU target - TARGET_MCU_MODS.
#
#    Target action part (optional, after second dot):
#       flash - flash specified mcu target binary
#
#   Unit test target: unittests_run
#

# Options
 # Valgrind
  VALGRIND ?= OFF

  ifneq ($(VALGRIND),ON)
   VALGRIND := OFF
  endif

export TARGET_DEBUG_MODES = debug
export TARGET_RELEASE_MODES = release
export TARGET_PC_SYSTEMS = linux

export TARGET_PC_MODS = cp cp_minimal mem_stats

export TARGET_MCU_MODS = cp cp_minimal

export TARGET_PC_SYSTEMS_MODS = $(TARGET_PC_SYSTEMS) \
                                $(foreach __MOD,$(TARGET_PC_MODS),$(foreach __SYSTEM,$(TARGET_PC_SYSTEMS),$(__SYSTEM)-$(__MOD)))
export TARGET_STM32F3_MODS = $(foreach __MOD,$(TARGET_MCU_MODS),mcu_stm32f3-$(__MOD))
export TARGET_STM32F4_MODS = $(foreach __MOD,$(TARGET_MCU_MODS),mcu_stm32f4-$(__MOD))

# Target list
export JERRY_LINUX_TARGETS = $(foreach __MODE,$(TARGET_DEBUG_MODES),$(foreach __SYSTEM,$(TARGET_PC_SYSTEMS_MODS),$(__MODE).$(__SYSTEM))) \
                             $(foreach __MODE,$(TARGET_RELEASE_MODES),$(foreach __SYSTEM,$(TARGET_PC_SYSTEMS_MODS),$(__MODE).$(__SYSTEM)))

export JERRY_STM32F3_TARGETS = $(foreach __MODE,$(TARGET_RELEASE_MODES),$(foreach __SYSTEM,$(TARGET_STM32F3_MODS),$(__MODE).$(__SYSTEM)))

export JERRY_STM32F4_TARGETS = $(foreach __MODE,$(TARGET_DEBUG_MODES),$(foreach __SYSTEM,$(TARGET_STM32F4_MODS),$(__MODE).$(__SYSTEM))) \
                               $(foreach __MODE,$(TARGET_RELEASE_MODES),$(foreach __SYSTEM,$(TARGET_STM32F4_MODS),$(__MODE).$(__SYSTEM)))

export JERRY_TARGETS = $(JERRY_LINUX_TARGETS) $(JERRY_STM32F3_TARGETS) $(JERRY_STM32F4_TARGETS)

export CHECK_TARGETS = $(foreach __TARGET,$(JERRY_LINUX_TARGETS),$(__TARGET).check)
export FLASH_TARGETS = $(foreach __TARGET,$(JERRY_STM32F3_TARGETS) $(JERRY_STM32F4_TARGETS),$(__TARGET).flash)

export OUT_DIR = ./build/bin

export SHELL=/bin/bash

# Building all options combinations
 OPTIONS_COMBINATIONS := $(foreach __OPTION,ON OFF,$(__COMBINATION)-VALGRIND-$(__OPTION))
 # OPTIONS_COMBINATIONS := $(foreach __COMBINATION,$(OPTIONS_COMBINATIONS),$(foreach __OPTION,ON OFF,$(__COMBINATION)-{ANOTHER_OPTION}-$(__OPTION)))

# Building current options string
 OPTIONS_STRING := -VALGRIND-$(VALGRIND)

# Build directories
 BUILD_DIR_PREFIX := ./build/obj

 # Native
  BUILD_DIRS_NATIVE := $(foreach _OPTIONS_COMBINATION,$(OPTIONS_COMBINATIONS),$(BUILD_DIR_PREFIX)$(_OPTIONS_COMBINATION)/native)
 # stm32f3
  BUILD_DIRS_STM32F3 := $(foreach _OPTIONS_COMBINATION,$(OPTIONS_COMBINATIONS),$(BUILD_DIR_PREFIX)$(_OPTIONS_COMBINATION)/stm32f3)
 # stm32f4
  BUILD_DIRS_STM32F4 := $(foreach _OPTIONS_COMBINATION,$(OPTIONS_COMBINATIONS),$(BUILD_DIR_PREFIX)$(_OPTIONS_COMBINATION)/stm32f4)

 # All together
  BUILD_DIRS_ALL := $(BUILD_DIRS_NATIVE) $(BUILD_DIRS_STM32F3) $(BUILD_DIRS_STM32F4)

 # Current
  BUILD_DIR := ./build/obj$(OPTIONS_STRING)

all: precommit

$(BUILD_DIRS_NATIVE):
	@ arch=`uname -p`; \
          if [ "$$arch" == "armv7l" ]; \
          then \
           readelf -A /proc/self/exe | grep Tag_ABI_VFP_args && arch=$$arch"-hf" || arch=$$arch"-el";\
          fi; \
	  mkdir -p $@ && \
          cd $@ && \
          cmake -DENABLE_VALGRIND=$(VALGRIND) -DCMAKE_TOOLCHAIN_FILE=build/configs/toolchain_linux_$$arch.cmake ../../.. &>cmake.log

$(BUILD_DIRS_STM32F3):
	@ mkdir -p $@ && \
          cd $@ && \
          cmake -DENABLE_VALGRIND=$(VALGRIND) -DCMAKE_TOOLCHAIN_FILE=build/configs/toolchain_mcu_stm32f3.cmake ../../.. &>cmake.log

$(BUILD_DIRS_STM32F4):
	@ mkdir -p $@ && \
          cd $@ && \
          cmake -DENABLE_VALGRIND=$(VALGRIND) -DCMAKE_TOOLCHAIN_FILE=build/configs/toolchain_mcu_stm32f4.cmake ../../.. &>cmake.log

$(JERRY_LINUX_TARGETS): $(BUILD_DIR)/native
	@ mkdir -p $(OUT_DIR)/$@
	@ $(MAKE) -C $(BUILD_DIR)/native VERBOSE=1 $@ &>$(OUT_DIR)/$@/make.log
	@ cp $(BUILD_DIR)/native/$@ $(OUT_DIR)/$@/jerry

unittests: $(BUILD_DIR)/native
	@ mkdir -p $(OUT_DIR)/$@
	@ $(MAKE) -C $(BUILD_DIR)/native VERBOSE=1 $@ &>$(OUT_DIR)/$@/make.log
	@ cp $(BUILD_DIR)/native/unit_test_* $(OUT_DIR)/$@

$(JERRY_STM32F3_TARGETS): $(BUILD_DIR)/stm32f3
	@ mkdir -p $(OUT_DIR)/$@
	@ $(MAKE) -C $(BUILD_DIR)/stm32f3 VERBOSE=1 $@.bin &>$(OUT_DIR)/$@/make.log
	@ cp $(BUILD_DIR)/stm32f3/$@ $(OUT_DIR)/$@/jerry
	@ cp $(BUILD_DIR)/stm32f3/$@.bin $(OUT_DIR)/$@/jerry.bin

$(JERRY_STM32F4_TARGETS): $(BUILD_DIR)/stm32f4
	@ mkdir -p $(OUT_DIR)/$@
	@ $(MAKE) -C $(BUILD_DIR)/stm32f4 VERBOSE=1 $@.bin &>$(OUT_DIR)/$@/make.log
	@ cp $(BUILD_DIR)/stm32f4/$@ $(OUT_DIR)/$@/jerry
	@ cp $(BUILD_DIR)/stm32f4/$@.bin $(OUT_DIR)/$@/jerry.bin

build: $(JERRY_TARGETS) unittests

$(FLASH_TARGETS): $(BUILD_DIR)/mcu
	@$(MAKE) -C $(BUILD_DIR)/mcu VERBOSE=1 $@ 1>/dev/null

PRECOMMIT_CHECK_TARGETS_NO_VALGRIND_LIST= debug.linux.check \
                                          release.linux.check

PRECOMMIT_CHECK_TARGETS_VALGRIND_LIST= #debug.linux-valgrind.check \
                                       release.linux-valgrind.check \
                                       release.linux-cp-valgrind.check

push: ./tools/git-scripts/push.sh
	@ ./tools/git-scripts/push.sh

pull: ./tools/git-scripts/pull.sh
	@ ./tools/git-scripts/pull.sh

log: ./tools/git-scripts/log.sh
	@ ./tools/git-scripts/log.sh

precommit: clean
	@ echo -e "\nBuilding...\n\n"
	@ $(MAKE) build
	@ echo -e "\n================ Build completed successfully. Running precommit tests ================\n"
	@ echo -e "All targets were built successfully. Starting unit tests' run.\n"
	@ $(MAKE) unittests_run TESTS_OPTS="--silent"
	@ echo -e "Unit tests completed successfully. Starting parse-only testing.\n"
	@ echo -e "All targets were built successfully. Starting parse-only testing.\n"
	@ # Parse-only testing
	@ for path in "./tests/jerry" "./tests/benchmarks/jerry"; \
          do \
            run_ids=""; \
            for check_target in $(PRECOMMIT_CHECK_TARGETS_NO_VALGRIND_LIST) $(PRECOMMIT_CHECK_TARGETS_VALGRIND_LIST); \
            do \
              $(MAKE) -s -f Makefile.mk TARGET=$$check_target $$check_target TESTS="$$path" TESTS_OPTS="--parse-only" OUTPUT_TO_LOG=enable & \
              run_ids="$$run_ids $$!"; \
            done; \
            result_ok=1; \
            for run_id in $$run_ids; \
            do \
              wait $$run_id || result_ok=0; \
            done; \
            [ $$result_ok -eq 1 ] || exit 1; \
          done
	@ echo -e "Parse-only testing completed successfully. Starting full tests run.\n"
	@ # Full testing
	@ for path in "./tests/jerry"; \
          do \
            run_ids=""; \
            for check_target in $(PRECOMMIT_CHECK_TARGETS_NO_VALGRIND_LIST) $(PRECOMMIT_CHECK_TARGETS_VALGRIND_LIST); \
            do \
              $(MAKE) -s -f Makefile.mk TARGET=$$check_target $$check_target TESTS="$$path" TESTS_OPTS="" OUTPUT_TO_LOG=enable & \
              run_ids="$$run_ids $$!"; \
            done; \
            result_ok=1; \
            for run_id in $$run_ids; \
            do \
              wait $$run_id || result_ok=0; \
            done; \
            [ $$result_ok -eq 1 ] || exit 1; \
            done
	@ for path in "./tests/jerry-test-suite/precommit_test_list"; \
          do \
            run_ids=""; \
            for check_target in $(PRECOMMIT_CHECK_TARGETS_NO_VALGRIND_LIST); \
            do \
              $(MAKE) -s -f Makefile.mk TARGET=$$check_target $$check_target TESTS="$$path" TESTS_OPTS="" OUTPUT_TO_LOG=enable & \
              run_ids="$$run_ids $$!"; \
            done; \
            result_ok=1; \
            for run_id in $$run_ids; \
            do \
              wait $$run_id || result_ok=0; \
            done; \
            [ $$result_ok -eq 1 ] || exit 1; \
            done
	@ echo -e "Full testing completed successfully\n\n================\n\n"

unittests_run: unittests
	@$(MAKE) -s -f Makefile.mk TARGET=$@ $@

clean:
	@ rm -rf $(BUILD_DIR_PREFIX)* $(OUT_DIR)

.PHONY: clean build unittests_run $(JERRY_TARGETS) $(FLASH_TARGETS)
