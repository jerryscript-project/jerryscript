#!/usr/bin/env python

# Copyright 2016 Samsung Electronics Co., Ltd.
# Copyright 2016 University of Szeged.
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

import argparse
import shutil
import subprocess
import sys
from os import makedirs, uname
from settings import *

BUILD_DIR = path.join(PROJECT_DIR, 'build')

def default_toolchain():
    (sysname, _, _, _, machine) = uname()
    toolchain = path.join(PROJECT_DIR, 'cmake', 'toolchain_%s_%s.cmake' % (sysname.lower(), machine.lower()))
    return toolchain if path.isfile(toolchain) else None

def add_build_args(parser):
    parser.add_argument('--verbose', '-v', action='store_const', const='ON', default='OFF', help='Increase verbosity')
    parser.add_argument('--unittests', action='store_const', const='ON', default='OFF', help='Build unittests too')
    parser.add_argument('--clean', action='store_true', default=False, help='Clean build')
    parser.add_argument('--builddir', action='store', default=BUILD_DIR, help='Specify output directory (default: %(default)s)')
    parser.add_argument('--strip', choices=['on', 'off'], default='on', help='Strip release binary (default: %(default)s)')
    parser.add_argument('--all-in-one', choices=['on', 'off'], default='off', help='All-in-one build (default: %(default)s)')
    parser.add_argument('--debug', action='store_const', const='Debug', default='Release', dest='build_type', help='Debug build')
    parser.add_argument('--lto', choices=['on', 'off'], default='on', help='Enable link-time optimizations (default: %(default)s)')
    parser.add_argument('--profile', choices=['full', 'minimal'], default='full', help='Specify the profile (default: %(default)s)')
    parser.add_argument('--error-messages', choices=['on', 'off'], default='off', help='Enable error messages (default: %(default)s)')
    parser.add_argument('--valgrind', choices=['on', 'off'], default='off', help='Enable Valgrind support (default: %(default)s)')
    parser.add_argument('--valgrind-freya', choices=['on', 'off'], default='off', help='Enable Valgrind-Freya support (default: %(default)s)')
    parser.add_argument('--show-opcodes', choices=['on', 'off'], default='off', help='Enable parser byte-code dumps (default: %(default)s)')
    parser.add_argument('--show-regexp-opcodes', choices=['on', 'off'], default='off', help='Enable regexp byte-code dumps (default: %(default)s)')
    parser.add_argument('--mem-stats', choices=['on', 'off'], default='off', help='Enable memory statistics (default: %(default)s)')
    parser.add_argument('--mem-stress-test', choices=['on', 'off'], default='off', help='Enable mem-stress test (default: %(default)s)')
    parser.add_argument('--snapshot-save', choices=['on', 'off'], default='on', help='Allow to save snapshot files (default: %(default)s)')
    parser.add_argument('--snapshot-exec', choices=['on', 'off'], default='on', help='Allow to execute snapshot files (default: %(default)s)')
    parser.add_argument('--cmake-param', action='append', default=[], help='Add custom arguments to CMake')
    parser.add_argument('--compile-flag', action='append', default=[], help='Add custom compile flag')
    parser.add_argument('--linker-flag', action='append', default=[], help='Add custom linker flag')
    parser.add_argument('--toolchain', action='store', default=default_toolchain(), help='Add toolchain file (default: %(default)s)')
    parser.add_argument('--jerry-libc', choices=['on', 'off'], default='on', help='Use jerry-libc (default: %(default)s)')
    parser.add_argument('--compiler-default-libc', choices=['on', 'off'], default='off', help='Use compiler-default libc (default: %(default)s)')
    parser.add_argument('--jerry-core', choices=['on', 'off'], default='on', help='Use jerry-core (default: %(default)s)')
    parser.add_argument('--jerry-libm', choices=['on', 'off'], default='on', help='Use jerry-libm (default: %(default)s)')
    parser.add_argument('--jerry-cmdline', choices=['on', 'off'], default='on', help='Use jerry commandline tool (default: %(default)s)')

def get_arguments():
    parser = argparse.ArgumentParser()
    add_build_args(parser)

    return parser.parse_args()

def generate_build_options(arguments):
    build_options = []

    build_options.append('-DJERRY_LIBC=%s' % arguments.jerry_libc.upper())
    build_options.append('-DJERRY_CORE=%s' % arguments.jerry_core.upper())
    build_options.append('-DJERRY_LIBM=%s' % arguments.jerry_libm.upper())
    build_options.append('-DJERRY_CMDLINE=%s' % arguments.jerry_cmdline.upper())
    build_options.append('-DCOMPILER_DEFAULT_LIBC=%s' % arguments.compiler_default_libc.upper())
    build_options.append('-DCMAKE_VERBOSE_MAKEFILE=%s' % arguments.verbose)
    build_options.append('-DCMAKE_BUILD_TYPE=%s' % arguments.build_type)
    build_options.append('-DFEATURE_PROFILE=%s' % arguments.profile)
    build_options.append('-DFEATURE_ERROR_MESSAGES=%s' % arguments.error_messages.upper())
    build_options.append('-DFEATURE_VALGRIND=%s' % arguments.valgrind.upper())
    build_options.append('-DFEATURE_VALGRIND_FREYA=%s' % arguments.valgrind_freya.upper())
    build_options.append('-DFEATURE_PARSER_DUMP=%s' % arguments.show_opcodes.upper())
    build_options.append('-DFEATURE_REGEXP_DUMP=%s' % arguments.show_regexp_opcodes.upper())
    build_options.append('-DFEATURE_MEM_STATS=%s' % arguments.mem_stats.upper())
    build_options.append('-DFEATURE_MEM_STRESS_TEST=%s' % arguments.mem_stress_test.upper())
    build_options.append('-DFEATURE_SNAPSHOT_SAVE=%s' % arguments.snapshot_save.upper())
    build_options.append('-DFEATURE_SNAPSHOT_EXEC=%s' % arguments.snapshot_exec.upper())
    build_options.append('-DENABLE_ALL_IN_ONE=%s' % arguments.all_in_one.upper())
    build_options.append('-DENABLE_LTO=%s' % arguments.lto.upper())
    build_options.append('-DENABLE_STRIP=%s' % arguments.strip.upper())
    build_options.append('-DUNITTESTS=%s' % arguments.unittests)

    build_options.extend(arguments.cmake_param)

    build_options.append('-DEXTERNAL_COMPILE_FLAGS=' + ' '.join(arguments.compile_flag))
    build_options.append('-DEXTERNAL_LINKER_FLAGS=' + ' '.join(arguments.linker_flag))

    if arguments.toolchain:
        build_options.append('-DCMAKE_TOOLCHAIN_FILE=%s' % arguments.toolchain)

    return build_options

def configure_output_dir(arguments):
    global BUILD_DIR

    if os.path.isabs(arguments.builddir):
        BUILD_DIR = arguments.builddir
    else:
        BUILD_DIR = path.join(PROJECT_DIR, arguments.builddir)

    if arguments.clean and os.path.exists(BUILD_DIR):
        shutil.rmtree(BUILD_DIR)

    if not os.path.exists(BUILD_DIR):
        makedirs(BUILD_DIR)

def configure_build(arguments):
    configure_output_dir(arguments)

    build_options = generate_build_options(arguments)

    cmake_cmd = ['cmake', '-B' + BUILD_DIR, '-H' + PROJECT_DIR]
    cmake_cmd.extend(build_options)

    return subprocess.call(cmake_cmd)

def build_jerry(arguments):
    return subprocess.call(['make', '--no-print-directory','-j', '-C', BUILD_DIR])

def print_result(ret):
    print('=' * 30)
    if ret:
        print('Build failed with exit code: %s' % (ret))
    else:
        print('Build succeeded!')
    print('=' * 30)

def main():
    arguments = get_arguments()
    ret = configure_build(arguments)

    if not ret:
        ret = build_jerry(arguments)

    print_result(ret)
    sys.exit(ret)


if __name__ == "__main__":
    main()
