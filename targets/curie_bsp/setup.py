#!/usr/bin/env python

# Copyright 2016 Intel Corporation
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
import sys
import os

project_name = 'curie_bsp_jerry'

# create soft link
def build_soft_links(curie_path, jerry_path):
    location_path = os.path.join(curie_path, 'wearable_device_sw/projects/' +
                                project_name)
    if not os.path.exists(location_path):
        os.makedirs(location_path)
    os.chdir(location_path)
    # arc
    if not os.path.islink(os.path.join(location_path, 'arc')):
        os.symlink(os.path.join(jerry_path,
                    'targets/curie_bsp/jerry_app/arc'), 'arc')

    # include
    if not os.path.islink(os.path.join(location_path, 'include')):
        os.symlink(os.path.join(jerry_path,
                    'targets/curie_bsp/jerry_app/include'), 'include')
    # quark
    if not os.path.islink(os.path.join(location_path, 'quark')):
        os.symlink(os.path.join(jerry_path,
                    'targets/curie_bsp/jerry_app/quark'), 'quark')

    # jerryscript
    location_path = os.path.join(location_path, 'quark')
    os.chdir(location_path)
    if not os.path.islink(os.path.join(location_path, 'jerryscript')):
        os.symlink(jerry_path, 'jerryscript')


# create Kbuild.mk
def breadth_first_travel(root_dir):
    out_put = ''
    lists = os.listdir(root_dir)
    dirs = []
    for line in lists:
        full_path = os.path.join(root_dir, line)
        if os.path.isdir(full_path):
            dirs.append(line)
        else:
            c_file = line.endswith('.c')
            if c_file:
                npos = line.find('.c')
                out_put += 'obj-y += ' + line[0:npos] + '.o\n'
                continue
            asm_file = line.endswith('.S')
            if asm_file:
                npos = line.find('.S')
                out_put += 'obj-y += ' + line[0:npos] + '.o\n'
    for line in dirs:
        out_put += 'obj-y += ' + line + '/\n'

    file_path = os.path.join(root_dir, 'Kbuild.mk')
    file_to_be_created = open(file_path, 'w+')
    file_to_be_created.write(out_put)
    file_to_be_created.close()
    for line in dirs:
        breadth_first_travel(os.path.join(root_dir, line))


# create Kbuild.mk in jerryscript/
def create_kbuild_in_jerryscript(jerry_path):
    breadth_first_travel(os.path.join(jerry_path, 'jerry-core'))
    breadth_first_travel(os.path.join(jerry_path, 'jerry-libm'))
    #jerryscript/
    out_put = '''
obj-y += jerry-core/
obj-y += jerry-libm/
obj-y += targets/
subdir-cflags-y += -DCONFIG_MEM_HEAP_AREA_SIZE=10*1024
subdir-cflags-y += -DJERRY_NDEBUG
subdir-cflags-y += -DJERRY_DISABLE_HEAVY_DEBUG
subdir-cflags-y += -DCONFIG_DISABLE_NUMBER_BUILTIN
subdir-cflags-y += -DCONFIG_DISABLE_STRING_BUILTIN
subdir-cflags-y += -DCONFIG_DISABLE_BOOLEAN_BUILTIN
#subdir-cflags-y += -DCONFIG_DISABLE_ERROR_BUILTINS
subdir-cflags-y += -DCONFIG_DISABLE_ARRAY_BUILTIN
subdir-cflags-y += -DCONFIG_DISABLE_MATH_BUILTIN
subdir-cflags-y += -DCONFIG_DISABLE_JSON_BUILTIN
subdir-cflags-y += -DCONFIG_DISABLE_DATE_BUILTIN
subdir-cflags-y += -DCONFIG_DISABLE_REGEXP_BUILTIN
subdir-cflags-y += -DCONFIG_DISABLE_ANNEXB_BUILTIN
subdir-cflags-y += -DCONFIG_ECMA_LCACHE_DISABLE
subdir-cflags-y += -DCONFIG_ECMA_PROPERTY_HASHMAP_DISABLE
'''

    file_path = os.path.join(jerry_path, 'Kbuild.mk')
    file_to_be_created = open(file_path, 'w+')
    file_to_be_created.write(out_put)
    file_to_be_created.close()

    # jerryscript/targets
    out_put =  'obj-y += curie_bsp/'
    file_path = os.path.join(jerry_path, 'targets/Kbuild.mk')
    file_to_be_created = open(file_path, 'w+')
    file_to_be_created.write(out_put)
    file_to_be_created.close()

    # jerryscript/targets/curie_bsp
    out_put =  'obj-y += source/'
    file_path = os.path.join(jerry_path, 'targets/curie_bsp/Kbuild.mk')
    file_to_be_created = open(file_path, 'w+')
    file_to_be_created.write(out_put)
    file_to_be_created.close()

    breadth_first_travel(os.path.join(jerry_path, 'targets/curie_bsp/source'))

# create Makefile in wearable_device_sw/projects/curie_bsp_jerry/
def create_makefile_in_curie(curie_path):

    # Kbuild.mk
    out_put = '''
obj-$(CONFIG_QUARK_SE_ARC) += arc/
obj-$(CONFIG_QUARK_SE_QUARK) += quark/
'''
    file_path = os.path.join(curie_path, 'wearable_device_sw/projects/' \
                                        + project_name + '/Kbuild.mk')

    file_to_be_created = open(file_path, 'w+')
    file_to_be_created.write(out_put)
    file_to_be_created.close()

    # Makefile
    out_put = '''
THIS_DIR   := $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST))))
T          := $(abspath $(THIS_DIR)/../..)
'''
    out_put += 'PROJECT      :=' + project_name
    out_put += '''
BOARD        := curie_101
ifeq ($(filter curie_101, $(BOARD)),)
$(error The curie jerry sample application can only run on the curie_101 Board)
endif
BUILDVARIANT ?= debug
quark_DEFCONFIG = $(PROJECT_PATH)/quark/defconfig
arc_DEFCONFIG = $(PROJECT_PATH)/arc/defconfig

# Optional: set the default version
VERSION_MAJOR  := 1
VERSION_MINOR  := 0
VERSION_PATCH  := 0
include $(T)/build/project.mk
'''
    file_path = os.path.join(curie_path, 'wearable_device_sw/projects/' \
                                        + project_name + '/Makefile')
    file_to_be_created = open(file_path, 'w+')
    file_to_be_created.write(out_put)
    file_to_be_created.close()

    # Kbuild.mk in arc/
    breadth_first_travel(os.path.join(curie_path, 'wearable_device_sw/projects/' \
                                                + project_name + '/arc/'))
    # Kbuild.mk in quark/
    file_path = os.path.join(curie_path, 'wearable_device_sw/projects/' \
                                        + project_name + '/quark/Kbuild.mk')
    out_put = '''
obj-y += jerryscript/
obj-y += main.o
subdir-cflags-y += -Wno-error
subdir-cflags-y += -I$(PROJECT_PATH)/quark/include
subdir-cflags-y += -I$(PROJECT_PATH)/quark/jerryscript
subdir-cflags-y += -I$(PROJECT_PATH)/quark/jerryscript/targets/curie_bsp/include
'''
    jerry_core_path = os.path.join(curie_path, 'wearable_device_sw/projects/' \
                                                + project_name \
                                                + '/quark/jerryscript/jerry-core/')
    for line in os.walk(jerry_core_path):
        npos = line[0].find(project_name)
        out_put += 'subdir-cflags-y += -I$(PROJECT_PATH)' \
                + line[0][npos + len(project_name):] + '\n'


    jerry_libm_path = os.path.join(curie_path, 'wearable_device_sw/projects/' \
                                                + project_name \
                                                + '/quark/jerryscript/jerry-libm/')

    for line in os.walk(jerry_libm_path):
        npos = line[0].find(project_name)
        out_put += 'subdir-cflags-y += -I$(PROJECT_PATH)' \
                + line[0][npos + len(project_name):] + '\n'

    file_to_be_created = open(file_path, 'w+')
    file_to_be_created.write(out_put)
    file_to_be_created.close()

def main(curie_path, jerry_path):
    build_soft_links(curie_path, jerry_path)
    create_kbuild_in_jerryscript(jerry_path)
    create_makefile_in_curie(curie_path)



if __name__ == '__main__':
    if len(sys.argv) != 2:
        print 'Usage:'
        print sys.argv[0] + ' [full or relative path of Curie_BSP]'
    else:
        file_dir = os.path.dirname(os.path.abspath(__file__))
        jerry_path = os.path.join(file_dir, "..", "..")
        curie_path = os.path.join(os.getcwd(), sys.argv[1])
        main(curie_path, jerry_path)
