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
#   Main targets: {debug,release}.{linux,stm32f{4}}[.flash]
#
#    Target mode part (before dot):
#       debug:         - JERRY_NDEBUG; - optimizations; + debug symbols; + -Werror  | debug build
#       release:       + JERRY_NDEBUG; + optimizations; - debug symbols; + -Werror  | release build
#
#    Target system and modifiers part (after first dot):
#       linux - target system is linux
#       mcu_stm32f{3,4} - target is STM32F{3,4} board
#
#       Modifiers can be added after '-' sign.
#        For list of modifiers for PC target - see TARGET_PC_MODS, for MCU target - TARGET_MCU_MODS.
#
#    Target action part (optional, after second dot):
#       flash - flash specified mcu target binary
#
#   Unit test target: unittests_run
#
# Parallel run
#   To build all targets in parallel, please, use make build -j
#   To run precommit in parallel mode, please, use make precommit -j
#
#   Parallel build of several selected targets started manually is not supported.
#

# Options
 # Valgrind
  VALGRIND ?= OFF

  ifneq ($(VALGRIND),ON)
   VALGRIND := OFF
  endif

 # Static checkers
  STATIC_CHECK ?= OFF

  ifneq ($(STATIC_CHECK),ON)
   STATIC_CHECK := OFF
  endif

 # LTO
  LTO ?= ON

  ifneq ($(LTO),ON)
   LTO := OFF
  endif

# External build configuration
 EXTERNAL_LIBS_INTERFACE ?= $(shell pwd)/third-party/nuttx/include
 EXTERNAL_C_COMPILER ?= arm-none-eabi-gcc
 EXTERNAL_CXX_COMPILER ?= arm-none-eabi-g++

export TARGET_DEBUG_MODES = debug
export TARGET_RELEASE_MODES = release

export TARGET_PC_SYSTEMS = linux
export TARGET_NUTTX_SYSTEMS = nuttx

export TARGET_PC_MODS = cp cp_minimal mem_stats
export TARGET_NUTTX_MODS = $(TARGET_PC_MODS)

export TARGET_MCU_MODS = cp cp_minimal

export TARGET_PC_SYSTEMS_MODS = $(TARGET_PC_SYSTEMS) \
                                $(foreach __MOD,$(TARGET_PC_MODS),$(foreach __SYSTEM,$(TARGET_PC_SYSTEMS),$(__SYSTEM)-$(__MOD)))
export TARGET_NUTTX_SYSTEMS_MODS = $(TARGET_NUTTX_SYSTEMS) \
                                   $(foreach __MOD,$(TARGET_NUTTX_MODS),$(foreach __SYSTEM,$(TARGET_NUTTX_SYSTEMS),$(__SYSTEM)-$(__MOD)))
export TARGET_STM32F3_MODS = $(foreach __MOD,$(TARGET_MCU_MODS),mcu_stm32f3-$(__MOD))
export TARGET_STM32F4_MODS = $(foreach __MOD,$(TARGET_MCU_MODS),mcu_stm32f4-$(__MOD))

# Target list
export JERRY_LINUX_TARGETS = $(foreach __MODE,$(TARGET_DEBUG_MODES),$(foreach __SYSTEM,$(TARGET_PC_SYSTEMS_MODS),$(__MODE).$(__SYSTEM))) \
                             $(foreach __MODE,$(TARGET_RELEASE_MODES),$(foreach __SYSTEM,$(TARGET_PC_SYSTEMS_MODS),$(__MODE).$(__SYSTEM)))
export JERRY_NUTTX_TARGETS = $(foreach __MODE,$(TARGET_DEBUG_MODES),$(foreach __SYSTEM,$(TARGET_NUTTX_SYSTEMS_MODS),$(__MODE).$(__SYSTEM))) \
                             $(foreach __MODE,$(TARGET_RELEASE_MODES),$(foreach __SYSTEM,$(TARGET_NUTTX_SYSTEMS_MODS),$(__MODE).$(__SYSTEM)))

export JERRY_STM32F3_TARGETS = $(foreach __MODE,$(TARGET_RELEASE_MODES),$(foreach __SYSTEM,$(TARGET_STM32F3_MODS),$(__MODE).$(__SYSTEM)))

export JERRY_STM32F4_TARGETS = $(foreach __MODE,$(TARGET_DEBUG_MODES),$(foreach __SYSTEM,$(TARGET_STM32F4_MODS),$(__MODE).$(__SYSTEM))) \
                               $(foreach __MODE,$(TARGET_RELEASE_MODES),$(foreach __SYSTEM,$(TARGET_STM32F4_MODS),$(__MODE).$(__SYSTEM)))

export JERRY_TARGETS = $(JERRY_LINUX_TARGETS) $(JERRY_NUTTX_TARGETS) $(JERRY_STM32F3_TARGETS) $(JERRY_STM32F4_TARGETS) unittests

export CHECK_TARGETS = $(foreach __TARGET,$(JERRY_LINUX_TARGETS),$(__TARGET).check)
export FLASH_TARGETS = $(foreach __TARGET,$(JERRY_STM32F3_TARGETS) $(JERRY_STM32F4_TARGETS),$(__TARGET).flash)

export OUT_DIR = ./build/bin
export PREREQUISITES_STATE_DIR = ./build/prerequisites

export SHELL=/bin/bash

# Precommit check targets
 PRECOMMIT_CHECK_TARGETS_LIST := debug.linux release.linux

# Building all options combinations
 OPTIONS_COMBINATIONS := $(foreach __OPTION,ON OFF,$(__COMBINATION)-VALGRIND-$(__OPTION))
 OPTIONS_COMBINATIONS := $(foreach __COMBINATION,$(OPTIONS_COMBINATIONS),$(foreach __OPTION,ON OFF,$(__COMBINATION)-LTO-$(__OPTION)))

# Building current options string
 OPTIONS_STRING := -VALGRIND-$(VALGRIND)-LTO-$(LTO)

# Build directories
 BUILD_DIR_PREFIX := ./build/obj

 # Native
  BUILD_DIRS_NATIVE := $(foreach _OPTIONS_COMBINATION,$(OPTIONS_COMBINATIONS),$(BUILD_DIR_PREFIX)$(_OPTIONS_COMBINATION)/native)
 # Nuttx
  BUILD_DIRS_NUTTX := $(foreach _OPTIONS_COMBINATION,$(OPTIONS_COMBINATIONS),$(BUILD_DIR_PREFIX)$(_OPTIONS_COMBINATION)/nuttx)
 # stm32f3
  BUILD_DIRS_STM32F3 := $(foreach _OPTIONS_COMBINATION,$(OPTIONS_COMBINATIONS),$(BUILD_DIR_PREFIX)$(_OPTIONS_COMBINATION)/stm32f3)
 # stm32f4
  BUILD_DIRS_STM32F4 := $(foreach _OPTIONS_COMBINATION,$(OPTIONS_COMBINATIONS),$(BUILD_DIR_PREFIX)$(_OPTIONS_COMBINATION)/stm32f4)

 # All together
  BUILD_DIRS_ALL := $(BUILD_DIRS_NATIVE) $(BUILD_DIRS_NUTTX) $(BUILD_DIRS_STM32F3) $(BUILD_DIRS_STM32F4)

 # Current
  BUILD_DIR := ./build/obj$(OPTIONS_STRING)

# "Build all" targets prefix
 BUILD_ALL := build_all

all: precommit

$(BUILD_DIRS_NATIVE): prerequisites
	@ arch=`uname -m`; \
          if [ "$$arch" == "armv7l" ]; \
          then \
           readelf -A /proc/self/exe | grep Tag_ABI_VFP_args && arch=$$arch"-hf" || arch=$$arch"-el";\
          fi; \
	  mkdir -p $@ && \
          cd $@ && \
          cmake -DENABLE_VALGRIND=$(VALGRIND) -DENABLE_LTO=$(LTO) -DCMAKE_TOOLCHAIN_FILE=build/configs/toolchain_linux_$$arch.cmake ../../.. &>cmake.log || \
          (echo "CMake run failed. See "`pwd`"/cmake.log for details."; exit 1;)

$(BUILD_DIRS_NUTTX): prerequisites
	@ [ "$(EXTERNAL_LIBS_INTERFACE)" != "" ] && [ -x `which $(EXTERNAL_C_COMPILER)` ] && [ -x `which $(EXTERNAL_CXX_COMPILER)` ] || \
          (echo "Wrong external arguments."; \
           echo "EXTERNAL_LIBS_INTERFACE='$(EXTERNAL_LIBS_INTERFACE)'"; \
           echo "EXTERNAL_C_COMPILER='$(EXTERNAL_C_COMPILER)'"; \
           echo "EXTERNAL_CXX_COMPILER='$(EXTERNAL_CXX_COMPILER)'"; \
           exit 1;)
	@ mkdir -p $@ && \
          cd $@ && \
          cmake \
          -DENABLE_VALGRIND=$(VALGRIND) -DENABLE_LTO=$(LTO) \
          -DCMAKE_TOOLCHAIN_FILE=build/configs/toolchain_external.cmake \
          -DEXTERNAL_LIBC_INTERFACE=${EXTERNAL_LIBS_INTERFACE} \
          -DEXTERNAL_CMAKE_C_COMPILER=${EXTERNAL_C_COMPILER} \
          -DEXTERNAL_CMAKE_CXX_COMPILER=${EXTERNAL_CXX_COMPILER} \
          ../../.. &>cmake.log || \
          (echo "CMake run failed. See "`pwd`"/cmake.log for details."; exit 1;)

$(BUILD_DIRS_STM32F3): prerequisites
	@ mkdir -p $@ && \
          cd $@ && \
          cmake -DENABLE_VALGRIND=$(VALGRIND) -DENABLE_LTO=$(LTO) -DCMAKE_TOOLCHAIN_FILE=build/configs/toolchain_mcu_stm32f3.cmake ../../.. &>cmake.log || \
          (echo "CMake run failed. See "`pwd`"/cmake.log for details."; exit 1;)

$(BUILD_DIRS_STM32F4): prerequisites
	@ mkdir -p $@ && \
          cd $@ && \
          cmake -DENABLE_VALGRIND=$(VALGRIND) -DENABLE_LTO=$(LTO) -DCMAKE_TOOLCHAIN_FILE=build/configs/toolchain_mcu_stm32f4.cmake ../../.. &>cmake.log || \
          (echo "CMake run failed. See "`pwd`"/cmake.log for details."; exit 1;)

$(JERRY_LINUX_TARGETS): $(BUILD_DIR)/native
	@ mkdir -p $(OUT_DIR)/$@
	@ [ "$(STATIC_CHECK)" = "OFF" ] || $(MAKE) -C $(BUILD_DIR)/native VERBOSE=1 cppcheck.$@ &>$(OUT_DIR)/$@/cppcheck.log || \
          (echo "cppcheck run failed. See $(OUT_DIR)/$@/cppcheck.log for details."; exit 1;)
	@ $(MAKE) -C $(BUILD_DIR)/native VERBOSE=1 $@ &>$(OUT_DIR)/$@/make.log || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)
	@ cp $(BUILD_DIR)/native/$@ $(OUT_DIR)/$@/jerry

unittests: $(BUILD_DIR)/native
	@ mkdir -p $(OUT_DIR)/$@
	@ [ "$(STATIC_CHECK)" = "OFF" ] || $(MAKE) -C $(BUILD_DIR)/native VERBOSE=1 cppcheck.$@ &>$(OUT_DIR)/$@/cppcheck.log || \
          (echo "cppcheck run failed. See $(OUT_DIR)/$@/cppcheck.log for details."; exit 1;)
	@ $(MAKE) -C $(BUILD_DIR)/native VERBOSE=1 $@ &>$(OUT_DIR)/$@/make.log || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)
	@ cp $(BUILD_DIR)/native/unit_test_* $(OUT_DIR)/$@

$(BUILD_ALL)_native: $(BUILD_DIRS_NATIVE)
	@ mkdir -p $(OUT_DIR)/$@
	@ $(MAKE) -C $(BUILD_DIR)/native jerry-libc-all VERBOSE=1 &>$(OUT_DIR)/$@/make.log || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)
	@ $(MAKE) -C $(BUILD_DIR)/native plugins-all VERBOSE=1 &>$(OUT_DIR)/$@/make.log || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)
	@ $(MAKE) -C $(BUILD_DIR)/native $(JERRY_LINUX_TARGETS) unittests VERBOSE=1 &>$(OUT_DIR)/$@/make.log || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)

$(JERRY_NUTTX_TARGETS): $(BUILD_DIR)/nuttx
	@ mkdir -p $(OUT_DIR)/$@
	@ [ "$(STATIC_CHECK)" = "OFF" ] || $(MAKE) -C $(BUILD_DIR)/nuttx VERBOSE=1 cppcheck.$@ &>$(OUT_DIR)/$@/cppcheck.log || \
          (echo "cppcheck run failed. See $(OUT_DIR)/$@/cppcheck.log for details."; exit 1;)
	@ $(MAKE) -C $(BUILD_DIR)/nuttx VERBOSE=1 $@ &>$(OUT_DIR)/$@/make.log || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)
	@ cp `cat $(BUILD_DIR)/nuttx/$@/list` $(OUT_DIR)/$@/

$(BUILD_ALL)_nuttx: $(BUILD_DIRS_NUTTX)
	@ mkdir -p $(OUT_DIR)/$@
	@ $(MAKE) -C $(BUILD_DIR)/nuttx plugins-all VERBOSE=1 &>$(OUT_DIR)/$@/make.log || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)
	@ $(MAKE) -C $(BUILD_DIR)/nuttx $(JERRY_NUTTX_TARGETS) VERBOSE=1 &>$(OUT_DIR)/$@/make.log || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)

$(JERRY_STM32F3_TARGETS): $(BUILD_DIR)/stm32f3
	@ mkdir -p $(OUT_DIR)/$@
	@ [ "$(STATIC_CHECK)" = "OFF" ] || $(MAKE) -C $(BUILD_DIR)/stm32f3 VERBOSE=1 cppcheck.$@ &>$(OUT_DIR)/$@/cppcheck.log || \
          (echo "cppcheck run failed. See $(OUT_DIR)/$@/cppcheck.log for details."; exit 1;)
	@ $(MAKE) -C $(BUILD_DIR)/stm32f3 VERBOSE=1 $@.bin &>$(OUT_DIR)/$@/make.log || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)
	@ cp $(BUILD_DIR)/stm32f3/$@ $(OUT_DIR)/$@/jerry
	@ cp $(BUILD_DIR)/stm32f3/$@.bin $(OUT_DIR)/$@/jerry.bin

$(BUILD_ALL)_stm32f3: $(BUILD_DIRS_STM32F3)
	@ mkdir -p $(OUT_DIR)/$@
	@ $(MAKE) -C $(BUILD_DIR)/stm32f3 jerry-libc-all VERBOSE=1 &>$(OUT_DIR)/$@/make.log || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)
	@ $(MAKE) -C $(BUILD_DIR)/stm32f3 plugins-all VERBOSE=1 &>$(OUT_DIR)/$@/make.log || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)
	@ $(MAKE) -C $(BUILD_DIR)/stm32f3 $(JERRY_STM32F3_TARGETS) VERBOSE=1 &>$(OUT_DIR)/$@/make.log || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)

$(JERRY_STM32F4_TARGETS): $(BUILD_DIR)/stm32f4
	@ mkdir -p $(OUT_DIR)/$@
	@ [ "$(STATIC_CHECK)" = "OFF" ] || $(MAKE) -C $(BUILD_DIR)/stm32f4 VERBOSE=1 cppcheck.$@ &>$(OUT_DIR)/$@/cppcheck.log || \
          (echo "cppcheck run failed. See $(OUT_DIR)/$@/cppcheck.log for details."; exit 1;)
	@ $(MAKE) -C $(BUILD_DIR)/stm32f4 VERBOSE=1 $@.bin &>$(OUT_DIR)/$@/make.log || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)
	@ cp $(BUILD_DIR)/stm32f4/$@ $(OUT_DIR)/$@/jerry
	@ cp $(BUILD_DIR)/stm32f4/$@.bin $(OUT_DIR)/$@/jerry.bin

$(BUILD_ALL)_stm32f4: $(BUILD_DIRS_STM32F4)
	@ mkdir -p $(OUT_DIR)/$@
	@ $(MAKE) -C $(BUILD_DIR)/stm32f4 jerry-libc-all VERBOSE=1 &>$(OUT_DIR)/$@/make.log || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)
	@ $(MAKE) -C $(BUILD_DIR)/stm32f4 plugins-all VERBOSE=1 &>$(OUT_DIR)/$@/make.log || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)
	@ $(MAKE) -C $(BUILD_DIR)/stm32f4 $(JERRY_STM32F4_TARGETS) VERBOSE=1 &>$(OUT_DIR)/$@/make.log || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)

build_all: $(BUILD_ALL)_native $(BUILD_ALL)_nuttx $(BUILD_ALL)_stm32f3 $(BUILD_ALL)_stm32f4

#
# build - build_all, then run cppcheck and copy output to OUT_DIR
# Prebuild is needed to avoid race conditions between make instances running in parallel
#
build: $(BUILD_ALL)
	@ mkdir -p $(OUT_DIR)/$@
	@ $(MAKE) VERBOSE=1 $(JERRY_TARGETS) &>$(OUT_DIR)/$@/make.log || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)
	@ rm -rf $(OUT_DIR)/$(BUILD_ALL)* $(OUT_DIR)/$@

$(FLASH_TARGETS): $(BUILD_DIR)/mcu
	@$(MAKE) -C $(BUILD_DIR)/mcu VERBOSE=1 $@ 1>/dev/null

push: ./tools/git-scripts/push.sh
	@ ./tools/git-scripts/push.sh

pull: ./tools/git-scripts/pull.sh
	@ ./tools/git-scripts/pull.sh

log: ./tools/git-scripts/log.sh
	@ ./tools/git-scripts/log.sh

precommit: clean prerequisites
	@ ./tools/precommit.sh "$(MAKE)" "$(OUT_DIR)" "$(PRECOMMIT_CHECK_TARGETS_LIST)"

unittests_run: unittests
	@rm -rf $(OUT_DIR)/unittests/check
	@mkdir -p $(OUT_DIR)/unittests/check
	@./tools/runners/run-unittests.sh $(OUT_DIR)/unittests || \
         (echo "Unit tests run failed. See $(OUT_DIR)/unittests/unit_tests_run.log for details."; exit 1;)

clean:
	@ rm -rf $(BUILD_DIR_PREFIX)* $(OUT_DIR)

prerequisites: $(PREREQUISITES_STATE_DIR)/.prerequisites

$(PREREQUISITES_STATE_DIR)/.prerequisites:
	@ echo "Setting up prerequisites... (log file: $(PREREQUISITES_STATE_DIR)/prerequisites.log)"
	@ mkdir -p $(PREREQUISITES_STATE_DIR)
	@ ./tools/prerequisites.sh $(PREREQUISITES_STATE_DIR)/.prerequisites >&$(PREREQUISITES_STATE_DIR)/prerequisites.log || \
          (echo "Prerequisites setup failed. See $(PREREQUISITES_STATE_DIR)/prerequisites.log for details."; exit 1;)
	@ echo "Prerequisites setup succeeded"

prerequisites_clean:
	@ ./tools/prerequisites.sh $(PREREQUISITES_STATE_DIR)/.prerequisites clean
	@ rm -rf $(PREREQUISITES_STATE_DIR)

.PHONY: prerequisites_clean prerequisites clean build unittests_run $(BUILD_DIRS_ALL) $(JERRY_TARGETS) $(FLASH_TARGETS)
