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

# default board: FRDM-K64F
BOARD ?= K64F

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

define MBED_CLI_FLAGS
	--clean
	--mbed-os-path $(MBED_OS_DIR)
	--program-path $(JERRY_ROOT_DIR)
	--toolchain $(TOOLCHAIN)
	--mbed-target $(BOARD)
	--app-config $(JERRY_ROOT_DIR)/mbed_lib.json
endef

.PHONY: all
all: .build

.PHONY: clean
clean: .extra-files-remove
	rm -rf $(JERRY_ROOT_DIR)/build/mbed-os

.PHONY: flash
flash: .mbed-set-flash-flag .build

.PHONY: .mbed-set-flash-flag
.mbed-set-flash-flag:
	$(eval MBED_CLI_FLAGS += --flash)

.PHONY: .build
.build: .extra-files-copy .mbed-build .extra-files-remove

.PHONY: .mbed-build
.mbed-build:
	mbed-tools compile $(strip $(MBED_CLI_FLAGS))

.PHONY: .extra-files-copy
.extra-files-copy:
	cp mbedignore.txt $(JERRY_ROOT_DIR)/.mbedignore
	cp mbed_lib.json $(JERRY_ROOT_DIR)/mbed_lib.json

.PHONY: .extra-files-remove
.extra-files-remove:
	rm -f $(JERRY_ROOT_DIR)/.mbedignore
	rm -f $(JERRY_ROOT_DIR)/mbed_lib.json
