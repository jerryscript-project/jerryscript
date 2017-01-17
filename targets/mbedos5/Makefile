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

# USAGE:
# specify the board using the command line:
# make BOARD=[mbed board name]

BOARD=$(subst [mbed] ,,$(shell mbed target))
HEAPSIZE=16

DEBUG=0
NO_JS=0
MBED_VERBOSE=0

MBED_CLI_FLAGS=-j0 --source . --source ../../

EXTRA_SRC=

ifneq ($(EXTRA_SRC),)
EXTRA_SRC_MOD=--source $(subst :, --source ,$(EXTRA_SRC))
MBED_CLI_FLAGS += $(EXTRA_SRC_MOD)
endif

EXTERN_BUILD_DIR=

ifneq ($(EXTERN_BUILD_DIR),)
MBED_CLI_FLAGS += --build $(EXTERN_BUILD_DIR)
endif

ifeq ($(DEBUG), 1)
MBED_CLI_FLAGS += --profile ./mbed-os/tools/profiles/debug.json
endif

ifeq ($(MBED_VERBOSE), 1)
MBED_CLI_FLAGS += -v
else ifeq ($(MBED_VERBOSE), 2)
MBED_CLI_FLAGS += -vv
endif

MBED_CLI_FLAGS += -D "CONFIG_MEM_HEAP_AREA_SIZE=(1024*$(HEAPSIZE))"
MBED_CLI_FLAGS += -t GCC_ARM

.PHONY: all js2c getlibs rebuild library
all: source/jerry-targetjs.h source/pins.cpp .mbed ../../.mbedignore
	mbed target $(BOARD)
	mbed compile $(MBED_CLI_FLAGS)

library: .mbed ../../.mbedignore
	# delete encoded js code if it exists
	rm -f source/jerry-targetjs.h
	mbed target $(BOARD)
	mbed compile $(MBED_CLI_FLAGS) --library

clean:
	rm -rf .build/$(BOARD)

js2c: js/main.js js/flash_leds.js
	python ../tools/js2c.py --ignore pins.js

source/pins.cpp:
	python tools/generate_pins.py ${BOARD}

ifeq ($(NO_JS),0)
source/jerry-targetjs.h: js2c
else
source/jerry-targetjs.h: ;
endif

getlibs: .mbed

.mbed:
	echo 'ROOT=.' > .mbed
	mbed config root .
	mbed toolchain GCC_ARM
	mbed target $(BOARD)
	mbed deploy

../../.mbedignore:
ifeq ($(OS),Windows_NT)
	copy template-mbedignore.txt ..\..\.mbedignore
else
	cp ./template-mbedignore.txt ../../.mbedignore
endif
