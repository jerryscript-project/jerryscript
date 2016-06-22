# Copyright 2014-2016 Samsung Electronics Co., Ltd.
# Copyright 2016 University of Szeged
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
#   Main targets: {debug,release}.{linux,darwin,mcu_stm32f{3,4}}
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
#        For list of modifiers for NATIVE target - see NATIVE_MODS, for MCU target - MCU_MODS.
#
#   Unit test target: test-unit
#
# Parallel run
#   To build all targets in parallel, please, use make build -j
#   To run precommit in parallel mode, please, use make precommit -j
#
#   Parallel build of several selected targets started manually is not supported.
#

export NATIVE_SYSTEM := $(shell uname -s | tr '[:upper:]' '[:lower:]')
export MCU_SYSTEMS := stm32f3 stm32f4

export DEBUG_MODES := debug
export RELEASE_MODES := release

export MCU_MODS := cp cp_minimal
export NATIVE_MODS := $(MCU_MODS) mem_stats mem_stress_test

# Options
CMAKE_DEFINES:=
BUILD_NAME:=

 # Valgrind
  ifneq ($(VALGRIND),)
   CMAKE_DEFINES:=$(CMAKE_DEFINES) -DENABLE_VALGRIND=$(VALGRIND)
   BUILD_NAME:=$(BUILD_NAME)-VALGRIND-$(VALGRIND)
  endif

 # Valgrind Freya
  ifneq ($(VALGRIND_FREYA),)
   CMAKE_DEFINES:=$(CMAKE_DEFINES) -DENABLE_VALGRIND_FREYA=$(VALGRIND_FREYA)
   BUILD_NAME:=$(BUILD_NAME)-VALGRIND_FREYA-$(VALGRIND_FREYA)
  endif

 # LTO
  ifneq ($(LTO),)
   CMAKE_DEFINES:=$(CMAKE_DEFINES) -DENABLE_LTO=$(LTO)
   BUILD_NAME:=$(BUILD_NAME)-LTO-$(LTO)
  endif

 # LOG
  ifneq ($(LOG),)
   CMAKE_DEFINES:=$(CMAKE_DEFINES) -DENABLE_LOG=$(LOG)
   BUILD_NAME:=$(BUILD_NAME)-LOG-$(LOG)
  endif

 # Fill error messages for builtin error objects
  ifneq ($(ERROR_MESSAGES),)
   CMAKE_DEFINES:=$(CMAKE_DEFINES) -DENABLE_ERROR_MESSAGES=$(ERROR_MESSAGES)
   BUILD_NAME:=$(BUILD_NAME)-ERROR_MESSAGES-$(ERROR_MESSAGES)
  endif

 # All-in-one build
  ifneq ($(ALL_IN_ONE),)
   CMAKE_DEFINES:=$(CMAKE_DEFINES) -DENABLE_ALL_IN_ONE=$(ALL_IN_ONE)
   BUILD_NAME:=$(BUILD_NAME)-ALL_IN_ONE-$(ALL_IN_ONE)
  endif

 # Flag, indicating whether to use compiler's default libc (ON / OFF)
  ifneq ($(COMPILER_DEFAULT_LIBC),)
   CMAKE_DEFINES:=$(CMAKE_DEFINES) -DCOMPILER_DEFAULT_LIBC=$(COMPILER_DEFAULT_LIBC)
   BUILD_NAME:=$(BUILD_NAME)-COMPILER_DEFAULT_LIBC-$(COMPILER_DEFAULT_LIBC)
  endif

 # Apply strip to release binaries
  ifneq ($(STRIP_RELEASE_BINARY),)
   CMAKE_DEFINES:=$(CMAKE_DEFINES) -DSTRIP_RELEASE_BINARY=$(STRIP_RELEASE_BINARY)
  endif

# For testing build-options
export BUILD_OPTIONS_TEST_NATIVE := LTO LOG ERROR_MESSAGES ALL_IN_ONE VALGRIND VALGRIND_FREYA COMPILER_DEFAULT_LIBC

# Directories
export ROOT_DIR := $(shell pwd)
export BUILD_DIR_PREFIX := $(ROOT_DIR)/build/obj
export BUILD_DIR := $(BUILD_DIR_PREFIX)$(BUILD_NAME)
export OUT_DIR := $(ROOT_DIR)/build/bin
export PREREQUISITES_STATE_DIR := $(ROOT_DIR)/build/prerequisites

SHELL := /bin/bash

# Build targets
export JERRY_NATIVE_TARGETS := \
  $(foreach __MODE,$(DEBUG_MODES) $(RELEASE_MODES), \
    $(__MODE).$(NATIVE_SYSTEM) \
    $(foreach __MOD,$(NATIVE_MODS), \
      $(__MODE).$(NATIVE_SYSTEM)-$(__MOD)))

export JERRY_STM32F3_TARGETS := \
  $(foreach __MODE,$(RELEASE_MODES), \
    $(__MODE).mcu_stm32f3 \
    $(foreach __MOD,$(MCU_MODS), \
      $(__MODE).mcu_stm32f3-$(__MOD)))

export JERRY_STM32F4_TARGETS := \
  $(foreach __MODE,$(DEBUG_MODES) $(RELEASE_MODES), \
    $(__MODE).mcu_stm32f4 \
    $(foreach __MOD,$(MCU_MODS), \
      $(__MODE).mcu_stm32f4-$(__MOD)))

# JS test targets (has to be a subset of JERRY_NATIVE_TARGETS)
export JERRY_TEST_TARGETS := \
  $(foreach __MODE,$(DEBUG_MODES) $(RELEASE_MODES), \
    $(__MODE).$(NATIVE_SYSTEM))

export JERRY_TEST_TARGETS_CP := \
  $(foreach __MODE,$(DEBUG_MODES) $(RELEASE_MODES), \
    $(__MODE).$(NATIVE_SYSTEM)-cp)

# Build-options test targets
export JERRY_BUILD_OPTIONS_TEST_TARGETS_NATIVE := \
  $(foreach __MODE,$(RELEASE_MODES), \
    $(__MODE).$(NATIVE_SYSTEM))

JERRY_BUILD_OPTIONS_TEST_TARGETS_NATIVE += unittests

# JS test suites (in the format of id:path)
export JERRY_TEST_SUITE_J := j:$(ROOT_DIR)/tests/jerry
export JERRY_TEST_SUITE_JTS := jts:$(ROOT_DIR)/tests/jerry-test-suite
export JERRY_TEST_SUITE_JTS_CP := jts-cp:$(ROOT_DIR)/tests/jerry-test-suite/compact-profile-list

# Default make target
.PHONY: all
all: precommit

# Verbosity control

ifdef VERBOSE
  Q :=
else
  Q := @
endif

# Shell command macro to invoke a command and redirect its output to a log file.
#
# $(1) - command to execute
# $(2) - log file to write (only in non-verbose mode)
# $(3) - command description (printed if command fails)
ifdef VERBOSE
define SHLOG
  $(1) || (echo -e "\e[1;33m$(3) failed. No log file generated. (Run make without VERBOSE if log is needed.)\e[0m"; exit 1;)
endef
else
define SHLOG
  ( mkdir -p $$(dirname $(2)) ; $(1) 2>&1 | tee $(2) >/dev/null ; ( exit $${PIPESTATUS[0]} ) ) || (echo -e "\e[1;33m$(3) failed. See $(2) for details.\e[0m"; exit 1;)
endef
endif

# Build system control
ifdef NINJA
  BUILD_GENERATOR := Ninja
  BUILD_COMMAND := ninja -v
else
  BUILD_GENERATOR := "Unix Makefiles"
  BUILD_COMMAND := $(MAKE) -w VERBOSE=1
endif

# Targets to prepare the build directories

# Shell command macro to write $TOOLCHAIN shell variable to toolchain.config
# file in the build directory, and clean it if found dirty.
#
# $(1) - build directory to write toolchain.config into
define WRITE_TOOLCHAIN_CONFIG
  if [ -d $(1) ]; \
  then \
    grep -s -q "$$TOOLCHAIN" $(1)/toolchain.config || rm -rf $(1) ; \
  fi; \
  mkdir -p $(1); \
  echo "$$TOOLCHAIN" > $(1)/toolchain.config
endef

.PHONY: $(BUILD_DIR)/$(NATIVE_SYSTEM)/toolchain.config
.PHONY: $(BUILD_DIR)/$(NATIVE_SYSTEM)/unittests/toolchain.config
$(BUILD_DIR)/$(NATIVE_SYSTEM)/toolchain.config \
$(BUILD_DIR)/$(NATIVE_SYSTEM)/unittests/toolchain.config:
	$(Q) if [ "$$TOOLCHAIN" == "" ]; \
          then \
            arch=`uname -m`; \
            if [ "$$arch" == "armv7l" ]; \
            then \
              readelf -A /proc/self/exe | grep Tag_ABI_VFP_args && arch=$$arch"-hf" || arch=$$arch"-el"; \
            fi; \
            TOOLCHAIN="build/configs/toolchain_$(NATIVE_SYSTEM)_$$arch.cmake"; \
          fi; \
          $(call WRITE_TOOLCHAIN_CONFIG,$(dir $@))

# Make rule macro to generate toolchain.config for MCUs.
#
# $(1) - mcu system name
define GEN_MCU_TOOLCHAIN_CONFIG_RULE
.PHONY: $$(BUILD_DIR)/$(1)/toolchain.config
$$(BUILD_DIR)/$(1)/toolchain.config: prerequisites
	$$(Q) TOOLCHAIN="build/configs/toolchain_mcu_$(1).cmake"; \
          $$(call WRITE_TOOLCHAIN_CONFIG,$$(BUILD_DIR)/$(1))
endef

$(foreach __SYSTEM,$(MCU_SYSTEMS), \
  $(eval $(call GEN_MCU_TOOLCHAIN_CONFIG_RULE,$(__SYSTEM))))

# Make rule macro to generate Makefile in build directory with cmake.
#
# $(1) - build directory to generate Makefile into
define GEN_MAKEFILE_RULE
.PHONY: $(1)/Makefile
$(1)/Makefile: $(1)/toolchain.config
	$$(Q) $$(call SHLOG,(cd $(1) && cmake -G $$(BUILD_GENERATOR) \
          $$(CMAKE_DEFINES) \
          -DCMAKE_TOOLCHAIN_FILE=`cat toolchain.config` $$(ROOT_DIR) 2>&1),$(1)/cmake.log,CMake run)
endef

$(foreach __SYSTEM,$(NATIVE_SYSTEM) $(MCU_SYSTEMS), \
  $(eval $(call GEN_MAKEFILE_RULE,$(BUILD_DIR)/$(__SYSTEM))))

$(eval $(call GEN_MAKEFILE_RULE,$(BUILD_DIR)/$(NATIVE_SYSTEM)/unittests))

# Targets to perform build and test steps in the build directories

# Make rule macro to build a/some target(s) and copy out the result(s).
#
# $(1) - rule to define in the current Makefile
# $(2) - name of the system which has a cmake-generated Makefile
# $(3) - target(s) to build with the cmake-generated Makefile
define BUILD_RULE
.PHONY: $(1)
$(1): $$(BUILD_DIR)/$(2)/Makefile
	$$(Q) $$(call SHLOG,$$(BUILD_COMMAND) -C $$(BUILD_DIR)/$(2) $(3),$$(BUILD_DIR)/$(2)/$(1).log,Build)
	$$(Q) $$(foreach __TARGET,$(3), \
            mkdir -p $$(OUT_DIR)/$$(__TARGET); \
            $$(if $$(findstring unittests,$$(__TARGET)), \
              cp $$(BUILD_DIR)/$(2)/unit-test-* $$(OUT_DIR)/$$(__TARGET); \
              , \
              $$(if $$(findstring .bin,$$(__TARGET)), \
                cp $$(BUILD_DIR)/$(2)/$$(__TARGET) $$(OUT_DIR)/$$(__TARGET)/jerry.bin; \
                cp $$(BUILD_DIR)/$(2)/$$(patsubst %.bin,%,$$(__TARGET)) $$(OUT_DIR)/$$(__TARGET)/jerry; \
                , \
                cp $$(BUILD_DIR)/$(2)/$$(__TARGET) $$(OUT_DIR)/$$(__TARGET)/jerry;)) \
          )
endef

$(foreach __TARGET,$(JERRY_NATIVE_TARGETS), \
  $(eval $(call BUILD_RULE,$(__TARGET),$(NATIVE_SYSTEM),$(__TARGET))))

$(eval $(call BUILD_RULE,build.$(NATIVE_SYSTEM),$(NATIVE_SYSTEM),$(JERRY_NATIVE_TARGETS)))

$(foreach __TARGET,$(JERRY_STM32F3_TARGETS), \
  $(eval $(call BUILD_RULE,$(__TARGET),stm32f3,$(__TARGET).bin)))

$(eval $(call BUILD_RULE,build.mcu_stm32f3,stm32f3,$(patsubst %,%.bin,$(JERRY_STM32F3_TARGETS))))

$(foreach __TARGET,$(JERRY_STM32F4_TARGETS), \
  $(eval $(call BUILD_RULE,$(__TARGET),stm32f4,$(__TARGET).bin)))

$(eval $(call BUILD_RULE,build.mcu_stm32f4,stm32f4,$(patsubst %,%.bin,$(JERRY_STM32F4_TARGETS))))

$(eval $(call BUILD_RULE,unittests,$(NATIVE_SYSTEM)/unittests,unittests))

# Make rule macro to test a build target with a test suite.
#
# $(1) - name of target to test
# $(2) - id of the test suite
# $(3) - path to the test suite
#
# FIXME: the dependency of the defined rule is sub-optimal, but if every rule
# would have its own proper dependency ($(1)), then potentially multiple builds
# would work in the same directory in parallel, and they would overwrite each
# others output. This manifests mostly in the repeated builds of jerry-libc and
# its non-deterministically vanishing .a files.
define JSTEST_RULE
test-js.$(1).$(2): build.$$(NATIVE_SYSTEM)
	$$(Q) mkdir -p $$(OUT_DIR)/$(1)/check/$(2)
	$$(Q) $$(call SHLOG,cd $$(OUT_DIR)/$(1)/check/$(2) && $$(ROOT_DIR)/tools/runners/run-test-suite.sh \
          $$(OUT_DIR)/$(1)/jerry \
          $(3),$$(OUT_DIR)/$(1)/check/$(2)/test.log,Testing)
test-js.$(1).$(2).snapshot: build.$$(NATIVE_SYSTEM)
	$$(Q) mkdir -p $$(OUT_DIR)/$(1)/check/$(2)
	$$(Q) $$(call SHLOG,cd $$(OUT_DIR)/$(1)/check/$(2) && $$(ROOT_DIR)/tools/runners/run-test-suite.sh \
          $$(OUT_DIR)/$(1)/jerry \
          $(3) --snapshot,$$(OUT_DIR)/$(1)/check/$(2)/snapshot.test.log,Testing)
endef

$(foreach __TARGET,$(JERRY_TEST_TARGETS), \
  $(foreach __SUITE,$(JERRY_TEST_SUITE_J) $(JERRY_TEST_SUITE_JTS), \
    $(eval $(call JSTEST_RULE,$(__TARGET),$(firstword $(subst :, ,$(__SUITE))),$(lastword $(subst :, ,$(__SUITE)))))))

$(foreach __TARGET,$(JERRY_TEST_TARGETS_CP), \
  $(foreach __SUITE,$(JERRY_TEST_SUITE_JTS_CP), \
    $(eval $(call JSTEST_RULE,$(__TARGET),$(firstword $(subst :, ,$(__SUITE))),$(lastword $(subst :, ,$(__SUITE)))))))

# Make rule macro to test a build target with a build option.
#
# $(1) - name of the target to test
# $(2) - name of the option to test
define OPTIONSTEST_RULE
.PHONY: test-option.$(1)-$(2)
test-option.$(1)-$(2):
	$$(Q)+$(MAKE) --no-print-directory $(2)=ON $(1)
endef

$(foreach __TARGET,$(JERRY_BUILD_OPTIONS_TEST_TARGETS_NATIVE), \
  $(foreach __OPTION, $(BUILD_OPTIONS_TEST_NATIVE), \
    $(eval $(call OPTIONSTEST_RULE,$(__TARGET),$(__OPTION)))))

# Targets to perform batch builds, checks, and tests

.PHONY: clean
clean:
	$(Q) rm -rf $(BUILD_DIR_PREFIX)* $(OUT_DIR)

.PHONY: check-signed-off
check-signed-off:
	$(Q) ./tools/check-signed-off.sh

.PHONY: build-zephyr
build-zephyr:
	$(Q) ./tools/build-zephyr.sh

.PHONY: check-vera
check-vera:
	$(Q) $(call SHLOG,./tools/check-vera.sh,$(OUT_DIR)/vera.log,Vera++)

.PHONY: check-cppcheck
check-cppcheck:
	$(Q) $(call SHLOG,./tools/check-cppcheck.sh,$(OUT_DIR)/cppcheck.log,Cppcheck)

.PHONY: build
build: build.$(NATIVE_SYSTEM) $(foreach __SYSTEM,$(MCU_SYSTEMS),build.mcu_$(__SYSTEM))

.PHONY: test-unit
test-unit: unittests
	$(Q) mkdir -p $(OUT_DIR)/unittests/check
	$(Q) $(call SHLOG,cd $(OUT_DIR)/unittests/check && $(ROOT_DIR)/tools/runners/run-unittests.sh $(OUT_DIR)/unittests,$(OUT_DIR)/unittests/check/unittests.log,Unit tests)

.PHONY: test-js
test-js: \
        $(foreach __TARGET,$(JERRY_TEST_TARGETS), \
          $(foreach __SUITE,$(JERRY_TEST_SUITE_J) $(JERRY_TEST_SUITE_JTS), \
            test-js.$(__TARGET).$(firstword $(subst :, ,$(__SUITE))) \
            test-js.$(__TARGET).$(firstword $(subst :, ,$(__SUITE))).snapshot)) \
        $(foreach __TARGET,$(JERRY_TEST_TARGETS_CP), \
          $(foreach __SUITE,$(JERRY_TEST_SUITE_JTS_CP), \
            test-js.$(__TARGET).$(firstword $(subst :, ,$(__SUITE))) \
            test-js.$(__TARGET).$(firstword $(subst :, ,$(__SUITE))).snapshot))

.PHONY: test-buildoptions
test-buildoptions: \
        $(foreach __TARGET,$(JERRY_BUILD_OPTIONS_TEST_TARGETS_NATIVE), \
          $(foreach __OPTION, $(BUILD_OPTIONS_TEST_NATIVE), \
            test-option.$(__TARGET)-$(__OPTION)))

.PHONY: precommit
precommit: prerequisites
	$(Q)+$(MAKE) --no-print-directory clean
	$(Q) echo "Running checks..."
	$(Q)+$(MAKE) --no-print-directory check-signed-off check-vera check-cppcheck
	$(Q) echo "...building engine..."
	$(Q)+$(MAKE) --no-print-directory build
	$(Q) echo "...building and running unit tests..."
	$(Q)+$(MAKE) --no-print-directory test-unit
	$(Q) echo "...running precommit JS tests..."
	$(Q)+$(MAKE) --no-print-directory test-js
	$(Q) echo "...SUCCESS"

# Targets to install and clean prerequisites

.PHONY: prerequisites
prerequisites:
	$(Q) mkdir -p $(PREREQUISITES_STATE_DIR)
	$(Q) $(call SHLOG,./tools/prerequisites.sh $(PREREQUISITES_STATE_DIR)/.prerequisites,$(PREREQUISITES_STATE_DIR)/prerequisites.log,Prerequisites setup)

.PHONY: prerequisites_clean
prerequisites_clean:
	$(Q) ./tools/prerequisites.sh $(PREREQUISITES_STATE_DIR)/.prerequisites clean
	$(Q) rm -rf $(PREREQUISITES_STATE_DIR)

# Git helper targets

.PHONY: push
push: ./tools/git-scripts/push.sh
	$(Q) ./tools/git-scripts/push.sh

.PHONY: pull
pull: ./tools/git-scripts/pull.sh
	$(Q) ./tools/git-scripts/pull.sh

.PHONY: log
log: ./tools/git-scripts/log.sh
	$(Q) ./tools/git-scripts/log.sh
