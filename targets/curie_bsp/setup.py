#!/usr/bin/env python

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
#
import fnmatch
import os

def build_soft_links(project_path, jerry_path):
    """ Creates soft links into the @project_path. """

    if not os.path.exists(project_path):
        os.makedirs(project_path)

    links = [
        { # arc
            'src': os.path.join('targets', 'curie_bsp', 'jerry_app', 'arc'),
            'link_name': 'arc'
        },
        { # include
            'src': os.path.join('targets', 'curie_bsp', 'jerry_app', 'include'),
            'link_name': 'include'
        },
        { # quark
            'src': os.path.join('targets', 'curie_bsp', 'jerry_app', 'quark'),
            'link_name': 'quark'
        },
        { # quark/jerryscript
            'src': jerry_path,
            'link_name': os.path.join('quark', 'jerryscript')
        }
    ]

    for link in links:
        src = os.path.join(jerry_path, link['src'])
        link_name = os.path.join(project_path, link['link_name'])
        if not os.path.islink(link_name):
            os.symlink(src, link_name)
            print("Created symlink '{link_name}' -> '{src}'".format(src=src, link_name=link_name))


def find_sources(root_dir, sub_dir):
    """
    Find .c and .S files inside the @root_dir/@sub_dir directory.
    Note: the returned paths will be relative to the @root_dir directory.
    """
    src_dir = os.path.join(root_dir, sub_dir)

    matches = []
    for root, dirnames, filenames in os.walk(src_dir):
        for filename in fnmatch.filter(filenames, '*.[c|S]'):
            file_path = os.path.join(root, filename)
            relative_path = os.path.relpath(file_path, root_dir)
            matches.append(relative_path)

    return matches


def build_jerry_data(jerry_path):
    """
    Build up a dictionary which contains the following items:
     - sources: list of JerryScript sources which should be built.
     - dirs: list of JerryScript dirs used.
     - cflags: CFLAGS for the build.
    """
    jerry_sources = []
    jerry_dirs = set()
    for sub_dir in ['jerry-core', 'jerry-libm', os.path.join('targets', 'curie_bsp', 'source')]:
        for file in find_sources(os.path.normpath(jerry_path), sub_dir):
            path = os.path.join('jerryscript', file)
            jerry_sources.append(path)
            jerry_dirs.add(os.path.split(path)[0])

    jerry_cflags = [
        '-DCONFIG_MEM_HEAP_AREA_SIZE=10*1024',
        '-DJERRY_NDEBUG',
        '-DJERRY_JS_PARSER',
        '-DJERRY_DISABLE_HEAVY_DEBUG',
        '-DCONFIG_DISABLE_NUMBER_BUILTIN',
        '-DCONFIG_DISABLE_STRING_BUILTIN',
        '-DCONFIG_DISABLE_BOOLEAN_BUILTIN',
        #'-DCONFIG_DISABLE_ERROR_BUILTINS',
        '-DCONFIG_DISABLE_ARRAY_BUILTIN',
        '-DCONFIG_DISABLE_MATH_BUILTIN',
        '-DCONFIG_DISABLE_JSON_BUILTIN',
        '-DCONFIG_DISABLE_DATE_BUILTIN',
        '-DCONFIG_DISABLE_REGEXP_BUILTIN',
        '-DCONFIG_DISABLE_ANNEXB_BUILTIN',
        '-DCONFIG_DISABLE_ARRAYBUFFER_BUILTIN',
        '-DCONFIG_DISABLE_TYPEDARRAY_BUILTIN',
        '-DCONFIG_ECMA_LCACHE_DISABLE',
        '-DCONFIG_ECMA_PROPERTY_HASHMAP_DISABLE',
    ]

    return {
        'sources': jerry_sources,
        'dirs': jerry_dirs,
        'cflags': jerry_cflags,
    }


def write_file(path, content):
    """ Writes @content into the file at specified by the @path. """
    norm_path = os.path.normpath(path)
    with open(norm_path, "w+") as f:
        f.write(content)
    print("Wrote file '{0}'".format(norm_path))


def build_obj_y(source_list):
    """
    Build obj-y additions from the @source_list.
    Note: the input sources should have their file extensions.
    """
    return '\n'.join(['obj-y += {0}.o'.format(os.path.splitext(fname)[0]) for fname in source_list])


def build_cflags_y(cflags_list):
    """
    Build cflags-y additions from the @cflags_list.
    Note: the input sources should have their file extensions.
    """
    return '\n'.join(['cflags-y += {0}'.format(cflag) for cflag in cflags_list])


def build_mkdir(dir_list):
    """ Build mkdir calls for each dir in the @dir_list. """
    return '\n'.join(['\t$(AT)mkdir -p {0}'.format(os.path.join('$(OUT_SRC)', path)) for path in dir_list])


def create_root_kbuild(project_path):
    """ Creates @project_path/Kbuild.mk file. """

    root_kbuild_path = os.path.join(project_path, 'Kbuild.mk')
    root_kbuild_content = '''
obj-$(CONFIG_QUARK_SE_ARC) += arc/
obj-$(CONFIG_QUARK_SE_QUARK) += quark/
'''
    write_file(root_kbuild_path, root_kbuild_content)


def create_root_makefile(project_path):
    """ Creates @project_path/Makefile file. """

    root_makefile_path = os.path.join(project_path, 'Makefile')
    root_makefile_content = '''
THIS_DIR   := $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST))))
T          := $(abspath $(THIS_DIR)/../..)
PROJECT    := {project_name}
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
'''.format(project_name=project_name)

    write_file(root_makefile_path, root_makefile_content)


def create_arc_kbuild(project_path):
    """ Creates @project_path/arc/Kbuild.mk file. """

    arc_path = os.path.join(project_path, 'arc')
    arc_kbuild_path = os.path.join(arc_path, 'Kbuild.mk')
    arc_sources = find_sources(arc_path, '.')
    arc_kbuild_content = build_obj_y(arc_sources)

    write_file(arc_kbuild_path, arc_kbuild_content)


def create_quark_kbuild(project_path, jerry_path):
    """ Creates @project_path/quark/Kbuild.mk file. """
    quark_kbuild_path = os.path.join(project_path, 'quark', 'Kbuild.mk')

    # Extract a few JerryScript related data
    jerry_data = build_jerry_data(jerry_path)
    jerry_objects = build_obj_y(jerry_data['sources'])
    jerry_defines = jerry_data['cflags']
    jerry_build_dirs = build_mkdir(jerry_data['dirs'])

    quark_include_paths = [
        'include',
        'jerryscript',
        os.path.join('jerryscript', 'jerry-libm', 'include'),
        os.path.join('jerryscript', 'targets' ,'curie_bsp', 'include')
    ] + list(jerry_data['dirs'])

    quark_includes = [
        '-Wno-error',
    ] + ['-I%s' % os.path.join(project_path, 'quark', path) for path in quark_include_paths]

    quark_cflags = build_cflags_y(jerry_defines + quark_includes)

    quark_kbuild_content = '''
{cflags}

obj-y += main.o
{objects}

build_dirs:
{dirs}

$(OUT_SRC): build_dirs
'''.format(objects=jerry_objects, cflags=quark_cflags, dirs=jerry_build_dirs)

    write_file(quark_kbuild_path, quark_kbuild_content)


def main(curie_path, project_name, jerry_path):
    project_path = os.path.join(curie_path, 'wearable_device_sw', 'projects', project_name)

    build_soft_links(project_path, jerry_path)

    create_root_kbuild(project_path)
    create_root_makefile(project_path)
    create_arc_kbuild(project_path)
    create_quark_kbuild(project_path, jerry_path)


if __name__ == '__main__':
    import sys

    if len(sys.argv) != 2:
        print('Usage:')
        print('{script_name} [full or relative path of Curie_BSP]'.format(script_name=sys.argv[0]))
        sys.exit(1)

    project_name = 'curie_bsp_jerry'

    file_dir = os.path.dirname(os.path.abspath(__file__))
    jerry_path = os.path.join(file_dir, "..", "..")
    curie_path = os.path.join(os.getcwd(), sys.argv[1])

    main(curie_path, project_name, jerry_path)
