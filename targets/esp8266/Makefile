# Copyright 2015-2016 Samsung Electronics Co., Ltd.
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

#############################################################
# Required variables for each makefile
# Discard this section from all parent makefiles
# Expected variables (with automatic defaults):
#   CSRCS (all "C" files in the dir)
#   SUBDIRS (all subdirs with a Makefile)
#   GEN_LIBS - list of libs to be generated ()
#   GEN_IMAGES - list of object file images to be generated ()
#   GEN_BINS - list of binaries to be generated ()
#   COMPONENTS_xxx - a list of libs/objs in the form
#     subdir/lib to be extracted and rolled up into
#     a generated lib/image xxx.a ()
#
# Tabsize : 8
#
TARGET = eagle
FLAVOR = release
#FLAVOR = debug

#EXTRA_CCFLAGS += -u

ifndef PDIR # {
GEN_IMAGES= eagle.app.v6.out
GEN_BINS= eagle.app.v6.bin
SPECIAL_MKTARGETS=$(APP_MKTARGETS)
SUBDIRS= user
endif # } PDIR

LDDIR = $(SDK_PATH)/ld

CCFLAGS += -Os
#CCFLAGS += -O0

TARGET_LDFLAGS =					\
	-nostdlib					\
	-Wl,-EL						\
	--longcalls					\
	--text-section-literals

ifeq ($(FLAVOR),debug)
	TARGET_LDFLAGS += -O0
endif

ifeq ($(FLAVOR),release)
	TARGET_LDFLAGS += -Os
endif

COMPONENTS_eagle.app.v6 =				\
	user/libuser.a					\


LINKFLAGS_eagle.app.v6 =				\
	-L$(SDK_PATH)/lib				\
	-Wl,--gc-sections				\
	-Wl,-Map,output.map				\
	-nostdlib					\
	-T$(LD_FILE)					\
	-Wl,--no-check-sections				\
	-u call_user_start				\
	-Wl,-static					\
	-Wl,--start-group				\
	-lminic						\
	-lgcc						\
	-lhal						\
	-lphy						\
	-lpp						\
	-lnet80211					\
	-lwpa						\
	-lmain						\
	-lfreertos					\
	-llwip						\
	-L./libs					\
	-ljerrycore					\
	-ljerryentry					\
	-ljerrylibm					\
	$(DEP_LIBS_eagle.app.v6)			\
	./libs/lib_a-setjmp.o				\
	-Wl,--end-group


DEPENDS_eagle.app.v6 =					\
	$(LD_FILE)					\
	$(LDDIR)/eagle.rom.addr.v6.ld			\
	./source/jerry-targetjs.h			\
	./libs/libjerrylibm.a				\
	./libs/libjerrycore.a				\
	./libs/libjerryentry.a


#############################################################
# Configuration i.e. compile options etc.
# Target specific stuff (defines etc.) goes in here!
# Generally values applying to a tree are captured in the
#   makefile at its root level - these are then overridden
#   for a subtree within the makefile rooted therein
#

#UNIVERSAL_TARGET_DEFINES =    \

# Other potential configuration flags include:
#  -DTXRX_TXBUF_DEBUG
#  -DTXRX_RXBUF_DEBUG
#  -DWLAN_CONFIG_CCX
CONFIGURATION_DEFINES = -DICACHE_FLASH

DEFINES += \
	$(UNIVERSAL_TARGET_DEFINES) \
	$(CONFIGURATION_DEFINES)

DDEFINES += \
	$(UNIVERSAL_TARGET_DEFINES) \
	$(CONFIGURATION_DEFINES)


#############################################################
# Recursion Magic - Don't touch this!!
#
# Each subtree potentially has an include directory
#   corresponding to the common APIs applicable to modules
#   rooted at that subtree. Accordingly, the INCLUDE PATH
#   of a module can only contain the include directories up
#   its parent path, and not its siblings
#
# Required for each makefile to inherit from the parent
#

INCLUDES := $(INCLUDES) -I $(PDIR)include -I $(PDIR)source
sinclude $(SDK_PATH)/Makefile

.PHONY: FORCE

FORCE:
