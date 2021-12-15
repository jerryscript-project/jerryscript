# Copyright JS Foundation and other contributors, http://js.foundation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# default board: STM32F4-Discovery
BOARD ?= DISCO_F407VG

# defult toolchain
TOOLCHAIN ?= GCC_ARM

# path to jerryscript/targets/os/mbedos5
JERRY_TARGET_DIR = $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

# path to the root jerryscript directory
JERRY_ROOT_DIR = $(realpath $(JERRY_TARGET_DIR)/../../..)

# default build path
BUILD_DIR ?= $(JERRY_ROOT_DIR)/build/mbed-os

# default jerry heap size (in KB)
JERRY_HEAP_SIZE ?= 70

.PHONY: all
all: .mbedignore-copy .mbed-build .mbedignore-remove

.PHONY: clean
clean:
	rm -rf $(JERRY_ROOT_DIR)/build/mbed-os

.PHONY: .mbed-build
.mbed-build:
	mbed config -G MBED_OS_DIR $(MBED_OS_DIR)
	mbed compile \
		--clean \
		--build $(BUILD_DIR) \
		--source $(MBED_OS_DIR) \
		--source $(JERRY_ROOT_DIR) \
		--source $(JERRY_TARGET_DIR) \
		--toolchain $(TOOLCHAIN) \
		--target $(BOARD) \
		--macro JERRY_GLOBAL_HEAP_SIZE=$(JERRY_HEAP_SIZE)

.mbedignore-copy:
	cp .mbedignore $(JERRY_ROOT_DIR)

.mbedignore-remove:
	rm $(JERRY_ROOT_DIR)/.mbedignore
