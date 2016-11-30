# Copyright JS Foundation and other contributors, http://js.foundation
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

# application name
APPLICATION = riot_jerry

# default BOARD enviroment
BOARD ?= stm32f4discovery

# path to the RIOT base directory
RIOTBASE ?= $(CURDIR)/../RIOT
# path to the JERRYSCRIPT directory
JERRYDIR ?= $(CURDIR)

# path to the application directory
APPDIR ?= $(JERRYDIR)/targets/riot-stm32f4/source

# path to the binary directory
BINDIR ?= $(JERRYDIR)/targets/riot-stm32f4/bin/

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
CFLAGS += -DDEVELHELP

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

INCLUDES += -I$(JERRYDIR)/jerry-core/

# Add the shell and some shell commands
USEMODULE += shell
USEMODULE += shell_commands

# Add the jerry lib
USEMODULE += libjerrycore

include $(RIOTBASE)/Makefile.include
