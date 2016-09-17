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
import multiprocessing
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

def get_arguments():
    devhelp_preparser = argparse.ArgumentParser(add_help=False)
    devhelp_preparser.add_argument('--devhelp', action='store_true', default=False, help='show help with all options (including those, which are useful for developers only)')

    devhelp_arguments, args = devhelp_preparser.parse_known_args()
    if devhelp_arguments.devhelp:
        args.append('--devhelp')

    def devhelp(help):
        return help if devhelp_arguments.devhelp else argparse.SUPPRESS

    parser = argparse.ArgumentParser(parents=[devhelp_preparser])
    parser.add_argument('-v', '--verbose', action='store_const', const='ON', default='OFF', help='increase verbosity')
    parser.add_argument('--clean', action='store_true', default=False, help='clean build')
    parser.add_argument('-j', '--jobs', metavar='N', action='store', type=int, default=multiprocessing.cpu_count() + 1, help='Allowed N build jobs at once (default: %(default)s)')
    parser.add_argument('--debug', action='store_const', const='Debug', default='Release', dest='build_type', help='debug build')
    parser.add_argument('--builddir', metavar='DIR', action='store', default=BUILD_DIR, help='specify output directory (default: %(default)s)')
    parser.add_argument('--lto', metavar='X', choices=['on', 'off'], default='on', help='enable link-time optimizations (%(choices)s; default: %(default)s)')
    parser.add_argument('--all-in-one', metavar='X', choices=['on', 'off'], default='off', help='all-in-one build (%(choices)s; default: %(default)s)')
    parser.add_argument('--profile', metavar='PROFILE', choices=['full', 'minimal'], default='full', help='specify the profile (%(choices)s; default: %(default)s)')
    parser.add_argument('--error-messages', metavar='X', choices=['on', 'off'], default='off', help='enable error messages (%(choices)s; default: %(default)s)')
    parser.add_argument('--snapshot-save', metavar='X', choices=['on', 'off'], default='off', help='enable saving snapshot files (%(choices)s; default: %(default)s)')
    parser.add_argument('--snapshot-exec', metavar='X', choices=['on', 'off'], default='off', help='enable executing snapshot files (%(choices)s; default: %(default)s)')
    parser.add_argument('--cpointer-32bit', metavar='X', choices=['on', 'off'], default='off', help='enable 32 bit compressed pointers (%(choices)s; default: %(default)s)')
    parser.add_argument('--toolchain', metavar='FILE', action='store', default=default_toolchain(), help='add toolchain file (default: %(default)s)')
    parser.add_argument('--cmake-param', metavar='OPT', action='append', default=[], help='add custom argument to CMake')
    parser.add_argument('--compile-flag', metavar='OPT', action='append', default=[], help='add custom compile flag')
    parser.add_argument('--linker-flag', metavar='OPT', action='append', default=[], help='add custom linker flag')
    parser.add_argument('--link-lib', metavar='OPT', action='append', default=[], help='add custom library to be linked')
    parser.add_argument('--jerry-libc', metavar='X', choices=['on', 'off'], default='on', help='build and use jerry-libc (%(choices)s; default: %(default)s)')
    parser.add_argument('--jerry-libm', metavar='X', choices=['on', 'off'], default='on', help='build and use jerry-libm (%(choices)s; default: %(default)s)')
    parser.add_argument('--jerry-cmdline', metavar='X', choices=['on', 'off'], default='on', help='build jerry command line tool (%(choices)s; default: %(default)s)')
    parser.add_argument('--static-link', metavar='X', choices=['on', 'off'], default='on', help='enable static linking of binaries (%(choices)s; default: %(default)s)')
    parser.add_argument('--strip', metavar='X', choices=['on', 'off'], default='on', help='strip release binaries (%(choices)s; default: %(default)s)')
    parser.add_argument('--unittests', action='store_const', const='ON', default='OFF', help='build unittests')

    devgroup = parser.add_argument_group('developer options')
    devgroup.add_argument('--valgrind', metavar='X', choices=['on', 'off'], default='off', help=devhelp('enable Valgrind support (%(choices)s; default: %(default)s)'))
    devgroup.add_argument('--valgrind-freya', metavar='X', choices=['on', 'off'], default='off', help=devhelp('enable Valgrind-Freya support (%(choices)s; default: %(default)s)'))
    devgroup.add_argument('--show-opcodes', metavar='X', choices=['on', 'off'], default='off', help=devhelp('enable parser byte-code dumps (%(choices)s; default: %(default)s)'))
    devgroup.add_argument('--show-regexp-opcodes', metavar='X', choices=['on', 'off'], default='off', help=devhelp('enable regexp byte-code dumps (%(choices)s; default: %(default)s)'))
    devgroup.add_argument('--mem-stats', metavar='X', choices=['on', 'off'], default='off', help=devhelp('enable memory statistics (%(choices)s; default: %(default)s)'))
    devgroup.add_argument('--mem-stress-test', metavar='X', choices=['on', 'off'], default='off', help=devhelp('enable mem-stress test (%(choices)s; default: %(default)s)'))
    devgroup.add_argument('--link-map', metavar='X', choices=['on', 'off'], default='off', help=devhelp('enable the generation of a link map file for jerry command line tool (%(choices)s; default: %(default)s)'))

    arguments = parser.parse_args(args)
    if arguments.devhelp:
        parser.print_help()
        sys.exit(0)

    return arguments

def generate_build_options(arguments):
    build_options = []

    build_options.append('-DJERRY_LIBC=%s' % arguments.jerry_libc.upper())
    build_options.append('-DJERRY_LIBM=%s' % arguments.jerry_libm.upper())
    build_options.append('-DJERRY_CMDLINE=%s' % arguments.jerry_cmdline.upper())
    build_options.append('-DCMAKE_VERBOSE_MAKEFILE=%s' % arguments.verbose)
    build_options.append('-DCMAKE_BUILD_TYPE=%s' % arguments.build_type)
    build_options.append('-DFEATURE_PROFILE=%s' % arguments.profile)
    build_options.append('-DFEATURE_ERROR_MESSAGES=%s' % arguments.error_messages.upper())
    build_options.append('-DFEATURE_VALGRIND=%s' % arguments.valgrind.upper())
    build_options.append('-DFEATURE_VALGRIND_FREYA=%s' % arguments.valgrind_freya.upper())
    build_options.append('-DFEATURE_PARSER_DUMP=%s' % arguments.show_opcodes.upper())
    build_options.append('-DFEATURE_REGEXP_DUMP=%s' % arguments.show_regexp_opcodes.upper())
    build_options.append('-DFEATURE_CPOINTER_32_BIT=%s' % arguments.cpointer_32bit.upper())
    build_options.append('-DFEATURE_MEM_STATS=%s' % arguments.mem_stats.upper())
    build_options.append('-DFEATURE_MEM_STRESS_TEST=%s' % arguments.mem_stress_test.upper())
    build_options.append('-DFEATURE_SNAPSHOT_SAVE=%s' % arguments.snapshot_save.upper())
    build_options.append('-DFEATURE_SNAPSHOT_EXEC=%s' % arguments.snapshot_exec.upper())
    build_options.append('-DENABLE_ALL_IN_ONE=%s' % arguments.all_in_one.upper())
    build_options.append('-DENABLE_LTO=%s' % arguments.lto.upper())
    build_options.append('-DENABLE_STRIP=%s' % arguments.strip.upper())
    build_options.append('-DUNITTESTS=%s' % arguments.unittests)
    build_options.append('-DENABLE_STATIC_LINK=%s' % arguments.static_link.upper())
    build_options.append('-DENABLE_LINK_MAP=%s' % arguments.link_map.upper())

    build_options.extend(arguments.cmake_param)

    build_options.append('-DEXTERNAL_COMPILE_FLAGS=' + ' '.join(arguments.compile_flag))
    build_options.append('-DEXTERNAL_LINKER_FLAGS=' + ' '.join(arguments.linker_flag))
    build_options.append('-DEXTERNAL_LINK_LIBS=' + ' '.join(arguments.link_lib))

    if arguments.toolchain:
        build_options.append('-DCMAKE_TOOLCHAIN_FILE=%s' % arguments.toolchain)

    return build_options

def configure_output_dir(arguments):
    global BUILD_DIR

    if path.isabs(arguments.builddir):
        BUILD_DIR = arguments.builddir
    else:
        BUILD_DIR = path.join(PROJECT_DIR, arguments.builddir)

    if arguments.clean and path.exists(BUILD_DIR):
        shutil.rmtree(BUILD_DIR)

    if not path.exists(BUILD_DIR):
        makedirs(BUILD_DIR)

def configure_build(arguments):
    configure_output_dir(arguments)

    build_options = generate_build_options(arguments)

    cmake_cmd = ['cmake', '-B' + BUILD_DIR, '-H' + PROJECT_DIR]
    cmake_cmd.extend(build_options)

    return subprocess.call(cmake_cmd)

def build_jerry(arguments):
    return subprocess.call(['make', '--no-print-directory','-j', str(arguments.jobs), '-C', BUILD_DIR])

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
