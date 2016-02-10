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
#   Main targets: {debug,release}.{linux,darwin,stm32f{4}}[.flash]
#
#    Target mode part (before dot):
#       debug:         - JERRY_NDEBUG; - optimizations; + debug symbols; + -Werror  | debug build
#       release:       + JERRY_NDEBUG; + optimizations; - debug symbols; + -Werror  | release build
#
#    Target system and modifiers part (after first dot):
#       linux - target system is linux
#       darwin - target system is Mac OS X
#       mcu_stm32f{3,4} - target is STM32F{3,4} board
#
#       Modifiers can be added after '-' sign.
#        For list of modifiers for NATIVE target - see TARGET_NATIVE_MODS, for MCU target - TARGET_MCU_MODS.
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

export TARGET_NATIVE_SYSTEMS = $(shell uname -s | tr '[:upper:]' '[:lower:]')

# Options
 # Valgrind
  VALGRIND ?= OFF

  ifneq ($(VALGRIND),ON)
   VALGRIND := OFF
  endif

 # Valgrind Freya
  VALGRIND_FREYA ?= OFF

  ifneq ($(VALGRIND_FREYA),ON)
   VALGRIND_FREYA := OFF
  endif

 # Static checkers
  STATIC_CHECK ?= OFF

  ifneq ($(STATIC_CHECK),ON)
   STATIC_CHECK := OFF
  endif

 # LTO
  ifeq ($(TARGET_NATIVE_SYSTEMS),darwin)
   LTO ?= OFF
  else
   LTO ?= ON
  endif

  ifneq ($(LTO),ON)
   LTO := OFF
  endif

 # LOG
  LOG ?= OFF
  ifneq ($(LOG),ON)
   LOG := OFF
  endif

 # All-in-one build
  ifeq ($(TARGET_NATIVE_SYSTEMS),darwin)
   ALL_IN_ONE ?= ON
  else
   ALL_IN_ONE ?= OFF
  endif

  ifneq ($(ALL_IN_ONE),ON)
   ALL_IN_ONE := OFF
  endif

 # Verbosity
  ifdef VERBOSE
   Q :=
   QLOG :=
  else
   Q := @
   QLOG := >/dev/null
  endif

# External build configuration
 # Flag, indicating whether to use compiler's default libc (YES / NO)
  USE_COMPILER_DEFAULT_LIBC ?= NO
 # List of include paths for external libraries (semicolon-separated)
  EXTERNAL_LIBS_INTERFACE ?=
 # External libc interface
  ifeq ($(USE_COMPILER_DEFAULT_LIBC),YES)
   ifneq ($(EXTERNAL_LIBC_INTERFACE),)
    $(error EXTERNAL_LIBC_INTERFACE should not be specified in case compiler's default libc is used)
   endif
  endif
 # Compiler to use for external build
 EXTERNAL_C_COMPILER ?= arm-none-eabi-gcc
 EXTERNAL_CXX_COMPILER ?= arm-none-eabi-g++

export TARGET_DEBUG_MODES = debug
export TARGET_RELEASE_MODES = release

export TARGET_NATIVE_MODS = cp cp_minimal mem_stats mem_stress_test

export TARGET_MCU_MODS = cp cp_minimal

export TARGET_NATIVE_SYSTEMS_MODS = $(TARGET_NATIVE_SYSTEMS) \
                                $(foreach __MOD,$(TARGET_NATIVE_MODS),$(foreach __SYSTEM,$(TARGET_NATIVE_SYSTEMS),$(__SYSTEM)-$(__MOD)))

export TARGET_STM32F3_MODS = $(foreach __MOD,$(TARGET_MCU_MODS),mcu_stm32f3-$(__MOD))
export TARGET_STM32F4_MODS = $(foreach __MOD,$(TARGET_MCU_MODS),mcu_stm32f4-$(__MOD))

# Target list
export JERRY_NATIVE_TARGETS = $(foreach __MODE,$(TARGET_DEBUG_MODES),$(foreach __SYSTEM,$(TARGET_NATIVE_SYSTEMS_MODS),$(__MODE).$(__SYSTEM))) \
                             $(foreach __MODE,$(TARGET_RELEASE_MODES),$(foreach __SYSTEM,$(TARGET_NATIVE_SYSTEMS_MODS),$(__MODE).$(__SYSTEM)))

export JERRY_STM32F3_TARGETS = $(foreach __MODE,$(TARGET_RELEASE_MODES),$(foreach __SYSTEM,$(TARGET_STM32F3_MODS),$(__MODE).$(__SYSTEM)))

export JERRY_STM32F4_TARGETS = $(foreach __MODE,$(TARGET_DEBUG_MODES),$(foreach __SYSTEM,$(TARGET_STM32F4_MODS),$(__MODE).$(__SYSTEM))) \
                               $(foreach __MODE,$(TARGET_RELEASE_MODES),$(foreach __SYSTEM,$(TARGET_STM32F4_MODS),$(__MODE).$(__SYSTEM)))

export JERRY_TARGETS = $(JERRY_NATIVE_TARGETS) $(JERRY_STM32F3_TARGETS) $(JERRY_STM32F4_TARGETS) unittests

export CHECK_TARGETS = $(foreach __TARGET,$(JERRY_NATIVE_TARGETS),$(__TARGET).check)
export FLASH_TARGETS = $(foreach __TARGET,$(JERRY_STM32F3_TARGETS) $(JERRY_STM32F4_TARGETS),$(__TARGET).flash)

export OUT_DIR = ./build/bin
export PREREQUISITES_STATE_DIR = ./build/prerequisites

export SHELL=/bin/bash

# Precommit check targets
 PRECOMMIT_CHECK_TARGETS_LIST := debug.$(TARGET_NATIVE_SYSTEMS) release.$(TARGET_NATIVE_SYSTEMS)

# Building all options combinations
 OPTIONS_COMBINATIONS := $(foreach __OPTION,ON OFF,$(__COMBINATION)-VALGRIND-$(__OPTION))
 OPTIONS_COMBINATIONS := $(foreach __COMBINATION,$(OPTIONS_COMBINATIONS),$(foreach __OPTION,ON OFF,$(__COMBINATION)-VALGRIND_FREYA-$(__OPTION)))
 OPTIONS_COMBINATIONS := $(foreach __COMBINATION,$(OPTIONS_COMBINATIONS),$(foreach __OPTION,ON OFF,$(__COMBINATION)-LTO-$(__OPTION)))
 OPTIONS_COMBINATIONS := $(foreach __COMBINATION,$(OPTIONS_COMBINATIONS),$(foreach __OPTION,ON OFF,$(__COMBINATION)-ALL_IN_ONE-$(__OPTION)))

# Building current options string
 OPTIONS_STRING := -VALGRIND-$(VALGRIND)-VALGRIND_FREYA-$(VALGRIND_FREYA)-LTO-$(LTO)-ALL_IN_ONE-$(ALL_IN_ONE)

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

# "Build all" targets prefix
 BUILD_ALL := build_all

.PHONY: all
all: precommit

.PHONY: $(BUILD_DIRS_NATIVE)
$(BUILD_DIRS_NATIVE):
	$(Q) if [ "$$TOOLCHAIN" == "" ]; \
          then \
            arch=`uname -m`; \
            if [ "$$arch" == "armv7l" ]; \
            then \
              readelf -A /proc/self/exe | grep Tag_ABI_VFP_args && arch=$$arch"-hf" || arch=$$arch"-el"; \
            fi; \
            TOOLCHAIN="build/configs/toolchain_$(TARGET_NATIVE_SYSTEMS)_$$arch.cmake"; \
          fi; \
          if [ -d "$@" ]; \
          then \
            grep -s -q "$$TOOLCHAIN" $@/toolchain.config || rm -rf $@ ; \
          fi; \
          mkdir -p $@; \
          echo "$$TOOLCHAIN" > $@/toolchain.config
	$(Q) cd $@ && \
          (cmake \
             -DENABLE_VALGRIND=$(VALGRIND) \
             -DENABLE_VALGRIND_FREYA=$(VALGRIND_FREYA) \
             -DENABLE_LOG=$(LOG) \
             -DENABLE_LTO=$(LTO) \
             -DENABLE_ALL_IN_ONE=$(ALL_IN_ONE) \
             -DUSE_COMPILER_DEFAULT_LIBC=$(USE_COMPILER_DEFAULT_LIBC) \
             -DCMAKE_TOOLCHAIN_FILE=`cat toolchain.config` ../../.. 2>&1 | tee cmake.log $(QLOG) ; ( exit $${PIPESTATUS[0]} ) ) || \
          (echo "CMake run failed. See "`pwd`"/cmake.log for details."; exit 1;); \

.PHONY: $(BUILD_DIRS_STM32F3)
$(BUILD_DIRS_STM32F3): prerequisites
	$(Q) mkdir -p $@
	$(Q) cd $@ && \
          (cmake -DENABLE_VALGRIND=$(VALGRIND) -DENABLE_VALGRIND_FREYA=$(VALGRIND_FREYA) -DENABLE_LTO=$(LTO) -DENABLE_ALL_IN_ONE=$(ALL_IN_ONE) -DCMAKE_TOOLCHAIN_FILE=build/configs/toolchain_mcu_stm32f3.cmake ../../.. 2>&1 | tee cmake.log $(QLOG) ; ( exit $${PIPESTATUS[0]} ) ) || \
          (echo "CMake run failed. See "`pwd`"/cmake.log for details."; exit 1;)

.PHONY: $(BUILD_DIRS_STM32F4)
$(BUILD_DIRS_STM32F4): prerequisites
	$(Q) mkdir -p $@
	$(Q) cd $@ && \
          (cmake -DENABLE_VALGRIND=$(VALGRIND) -DENABLE_VALGRIND_FREYA=$(VALGRIND_FREYA) -DENABLE_LTO=$(LTO) -DENABLE_ALL_IN_ONE=$(ALL_IN_ONE) -DCMAKE_TOOLCHAIN_FILE=build/configs/toolchain_mcu_stm32f4.cmake ../../.. 2>&1 | tee cmake.log $(QLOG) ; ( exit $${PIPESTATUS[0]} ) ) || \
          (echo "CMake run failed. See "`pwd`"/cmake.log for details."; exit 1;)

.PHONY: $(JERRY_NATIVE_TARGETS)
$(JERRY_NATIVE_TARGETS): $(BUILD_DIR)/native
	$(Q) mkdir -p $(OUT_DIR)/$@
	$(Q) [ "$(STATIC_CHECK)" = "OFF" ] || ($(MAKE) -C $(BUILD_DIR)/native VERBOSE=1 cppcheck.$@ 2>&1 | tee $(OUT_DIR)/$@/cppcheck.log $(QLOG) ; ( exit $${PIPESTATUS[0]} ) ) || \
          (echo "cppcheck run failed. See $(OUT_DIR)/$@/cppcheck.log for details."; exit 1;)
	$(Q) ($(MAKE) -C $(BUILD_DIR)/native VERBOSE=1 $@ 2>&1 | tee $(OUT_DIR)/$@/make.log $(QLOG) ; ( exit $${PIPESTATUS[0]} ) ) || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)
	$(Q) cp $(BUILD_DIR)/native/$@ $(OUT_DIR)/$@/jerry

.PHONY: unittests
unittests: $(BUILD_DIR)/native
	$(Q) mkdir -p $(OUT_DIR)/$@
	$(Q) [ "$(STATIC_CHECK)" = "OFF" ] || ($(MAKE) -C $(BUILD_DIR)/native VERBOSE=1 cppcheck.$@ 2>&1 | tee $(OUT_DIR)/$@/cppcheck.log $(QLOG) ; ( exit $${PIPESTATUS[0]} ) ) || \
          (echo "cppcheck run failed. See $(OUT_DIR)/$@/cppcheck.log for details."; exit 1;)
	$(Q) ($(MAKE) -C $(BUILD_DIR)/native VERBOSE=1 $@ 2>&1 | tee $(OUT_DIR)/$@/make.log $(QLOG) ; ( exit $${PIPESTATUS[0]} ) ) || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)
	$(Q) cp $(BUILD_DIR)/native/unit-test-* $(OUT_DIR)/$@

.PHONY: $(BUILD_ALL)_native
$(BUILD_ALL)_native: $(BUILD_DIRS_NATIVE)
	$(Q) mkdir -p $(OUT_DIR)/$@
	$(Q) [ "$(USE_COMPILER_DEFAULT_LIBC)" = "YES" ] || ($(MAKE) -C $(BUILD_DIR)/native jerry-libc-all VERBOSE=1 2>&1 | tee $(OUT_DIR)/$@/make.log $(QLOG) ; ( exit $${PIPESTATUS[0]} ) ) || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)
	$(Q) ($(MAKE) -C $(BUILD_DIR)/native jerry-fdlibm-all VERBOSE=1 2>&1 | tee $(OUT_DIR)/$@/make.log $(QLOG) ; ( exit $${PIPESTATUS[0]} ) ) || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)
	$(Q) ($(MAKE) -C $(BUILD_DIR)/native $(JERRY_NATIVE_TARGETS) unittests VERBOSE=1 2>&1 | tee $(OUT_DIR)/$@/make.log $(QLOG) ; ( exit $${PIPESTATUS[0]} ) ) || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)

.PHONY: $(JERRY_STM32F3_TARGETS)
$(JERRY_STM32F3_TARGETS): $(BUILD_DIR)/stm32f3
	$(Q) mkdir -p $(OUT_DIR)/$@
	$(Q) [ "$(STATIC_CHECK)" = "OFF" ] || ($(MAKE) -C $(BUILD_DIR)/stm32f3 VERBOSE=1 cppcheck.$@ 2>&1 | tee $(OUT_DIR)/$@/cppcheck.log $(QLOG) ; ( exit $${PIPESTATUS[0]} ) ) || \
          (echo "cppcheck run failed. See $(OUT_DIR)/$@/cppcheck.log for details."; exit 1;)
	$(Q) ($(MAKE) -C $(BUILD_DIR)/stm32f3 VERBOSE=1 $@.bin 2>&1 | tee $(OUT_DIR)/$@/make.log $(QLOG) ; ( exit $${PIPESTATUS[0]} ) ) || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)
	$(Q) cp $(BUILD_DIR)/stm32f3/$@ $(OUT_DIR)/$@/jerry
	$(Q) cp $(BUILD_DIR)/stm32f3/$@.bin $(OUT_DIR)/$@/jerry.bin

.PHONY: $(BUILD_ALL)_stm32f3
$(BUILD_ALL)_stm32f3: $(BUILD_DIRS_STM32F3)
	$(Q) mkdir -p $(OUT_DIR)/$@
	$(Q) ($(MAKE) -C $(BUILD_DIR)/stm32f3 jerry-libc-all VERBOSE=1 2>&1 | tee $(OUT_DIR)/$@/make.log $(QLOG) ; ( exit $${PIPESTATUS[0]} ) ) || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)
	$(Q) ($(MAKE) -C $(BUILD_DIR)/stm32f3 $(JERRY_STM32F3_TARGETS) VERBOSE=1 2>&1 | tee $(OUT_DIR)/$@/make.log $(QLOG) ; ( exit $${PIPESTATUS[0]} ) ) || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)

.PHONY: $(JERRY_STM32F4_TARGETS)
$(JERRY_STM32F4_TARGETS): $(BUILD_DIR)/stm32f4
	$(Q) mkdir -p $(OUT_DIR)/$@
	$(Q) [ "$(STATIC_CHECK)" = "OFF" ] || ($(MAKE) -C $(BUILD_DIR)/stm32f4 VERBOSE=1 cppcheck.$@ 2>&1 | tee $(OUT_DIR)/$@/cppcheck.log $(QLOG) ; ( exit $${PIPESTATUS[0]} ) ) || \
          (echo "cppcheck run failed. See $(OUT_DIR)/$@/cppcheck.log for details."; exit 1;)
	$(Q) ($(MAKE) -C $(BUILD_DIR)/stm32f4 VERBOSE=1 $@.bin 2>&1 | tee $(OUT_DIR)/$@/make.log $(QLOG) ; ( exit $${PIPESTATUS[0]} ) ) || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)
	$(Q) cp $(BUILD_DIR)/stm32f4/$@ $(OUT_DIR)/$@/jerry
	$(Q) cp $(BUILD_DIR)/stm32f4/$@.bin $(OUT_DIR)/$@/jerry.bin

.PHONY: $(BUILD_ALL)_stm32f4
$(BUILD_ALL)_stm32f4: $(BUILD_DIRS_STM32F4)
	$(Q) mkdir -p $(OUT_DIR)/$@
	$(Q) ($(MAKE) -C $(BUILD_DIR)/stm32f4 jerry-libc-all VERBOSE=1 2>&1 | tee $(OUT_DIR)/$@/make.log $(QLOG) ; ( exit $${PIPESTATUS[0]} ) ) || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)
	$(Q) ($(MAKE) -C $(BUILD_DIR)/stm32f4 $(JERRY_STM32F4_TARGETS) VERBOSE=1 2>&1 | tee $(OUT_DIR)/$@/make.log $(QLOG) ; ( exit $${PIPESTATUS[0]} ) ) || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)

.PHONY: $(BUILD_ALL)
$(BUILD_ALL): $(BUILD_ALL)_native $(BUILD_ALL)_stm32f3 $(BUILD_ALL)_stm32f4

#
# build - build_all, then run cppcheck and copy output to OUT_DIR
# Prebuild is needed to avoid race conditions between make instances running in parallel
#
.PHONY: build
build: $(BUILD_ALL)
	$(Q) mkdir -p $(OUT_DIR)/$@
	$(Q) ($(MAKE) VERBOSE=1 $(JERRY_TARGETS) 2>&1 | tee $(OUT_DIR)/$@/make.log $(QLOG) ; ( exit $${PIPESTATUS[0]} ) ) || \
          (echo "Build failed. See $(OUT_DIR)/$@/make.log for details."; exit 1;)
	$(Q) rm -rf $(OUT_DIR)/$(BUILD_ALL)* $(OUT_DIR)/$@

.PHONY: $(FLASH_TARGETS)
$(FLASH_TARGETS): $(BUILD_DIR)/mcu
	$(Q) $(MAKE) -C $(BUILD_DIR)/mcu VERBOSE=1 $@ $(QLOG)

.PHONY: push
push: ./tools/git-scripts/push.sh
	$(Q) ./tools/git-scripts/push.sh

.PHONY: pull
pull: ./tools/git-scripts/pull.sh
	$(Q) ./tools/git-scripts/pull.sh

.PHONY: log
log: ./tools/git-scripts/log.sh
	$(Q) ./tools/git-scripts/log.sh

.PHONY: precommit
precommit: clean prerequisites
	$(Q) ./tools/precommit.sh "$(MAKE)" "$(OUT_DIR)" "$(PRECOMMIT_CHECK_TARGETS_LIST)"

.PHONY: unittests_run
unittests_run: unittests
	$(Q) rm -rf $(OUT_DIR)/unittests/check
	$(Q) mkdir -p $(OUT_DIR)/unittests/check
	$(Q) ./tools/runners/run-unittests.sh $(OUT_DIR)/unittests || \
         (echo "Unit tests run failed. See $(OUT_DIR)/unittests/unit_tests_run.log for details."; exit 1;)

.PHONY: clean
clean:
	$(Q) rm -rf $(BUILD_DIR_PREFIX)* $(OUT_DIR)

.PHONY: prerequisites
prerequisites:
	$(Q) mkdir -p $(PREREQUISITES_STATE_DIR)
	$(Q) (./tools/prerequisites.sh $(PREREQUISITES_STATE_DIR)/.prerequisites 2>&1 | tee $(PREREQUISITES_STATE_DIR)/prerequisites.log $(QLOG) ; ( exit $${PIPESTATUS[0]} ) ) || \
          (echo "Prerequisites setup failed. See $(PREREQUISITES_STATE_DIR)/prerequisites.log for details."; exit 1;)

.PHONY: prerequisites_clean
prerequisites_clean:
	$(Q) ./tools/prerequisites.sh $(PREREQUISITES_STATE_DIR)/.prerequisites clean
	$(Q) rm -rf $(PREREQUISITES_STATE_DIR)
