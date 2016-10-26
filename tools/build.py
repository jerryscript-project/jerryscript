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

import argparse
import multiprocessing
import shutil
import subprocess
import sys
from os import makedirs, uname
from settings import *

BUILD_DIR = path.join(PROJECT_DIR, 'build')
DEFAULT_PORT_DIR = path.join(PROJECT_DIR, 'targets/default')

PROFILE_DIR = path.join(PROJECT_DIR, 'jerry-core/profiles')
DEFAULT_PROFILE = 'es5.1'

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
    parser.add_argument('--all-in-one', metavar='X', choices=['ON', 'OFF'], default='OFF', type=str.upper, help='all-in-one build (%(choices)s; default: %(default)s)')
    parser.add_argument('--builddir', metavar='DIR', action='store', default=BUILD_DIR, help='specify output directory (default: %(default)s)')
    parser.add_argument('--clean', action='store_true', default=False, help='clean build')
    parser.add_argument('--cmake-param', metavar='OPT', action='append', default=[], help='add custom argument to CMake')
    parser.add_argument('--compile-flag', metavar='OPT', action='append', default=[], help='add custom compile flag')
    parser.add_argument('--cpointer-32bit', metavar='X', choices=['ON', 'OFF'], default='OFF', type=str.upper, help='enable 32 bit compressed pointers (%(choices)s; default: %(default)s)')
    parser.add_argument('--debug', action='store_const', const='Debug', default='MinSizeRel', dest='build_type', help='debug build')
    parser.add_argument('--error-messages', metavar='X', choices=['ON', 'OFF'], default='OFF', type=str.upper, help='enable error messages (%(choices)s; default: %(default)s)')
    parser.add_argument('-j', '--jobs', metavar='N', action='store', type=int, default=multiprocessing.cpu_count() + 1, help='Allowed N build jobs at once (default: %(default)s)')
    parser.add_argument('--jerry-cmdline', metavar='X', choices=['ON', 'OFF'], default='ON', type=str.upper, help='build jerry command line tool (%(choices)s; default: %(default)s)')
    parser.add_argument('--jerry-cmdline-minimal', metavar='X', choices=['ON', 'OFF'], default='OFF', type=str.upper, help='build minimal version of the jerry command line tool (%(choices)s; default: %(default)s)')
    parser.add_argument('--jerry-debugger', metavar='X', choices=['ON', 'OFF'], default='OFF', type=str.upper, help='enable the jerry debugger (%(choices)s; default: %(default)s)')
    parser.add_argument('--jerry-libc', metavar='X', choices=['ON', 'OFF'], default='ON', type=str.upper, help='build and use jerry-libc (%(choices)s; default: %(default)s)')
    parser.add_argument('--jerry-libm', metavar='X', choices=['ON', 'OFF'], default='ON', type=str.upper, help='build and use jerry-libm (%(choices)s; default: %(default)s)')
    parser.add_argument('--js-parser', metavar='X', choices=['ON', 'OFF'], default='ON', type=str.upper, help='enable js-parser (%(choices)s; default: %(default)s)')
    parser.add_argument('--link-lib', metavar='OPT', action='append', default=[], help='add custom library to be linked')
    parser.add_argument('--linker-flag', metavar='OPT', action='append', default=[], help='add custom linker flag')
    parser.add_argument('--lto', metavar='X', choices=['ON', 'OFF'], default='ON', type=str.upper, help='enable link-time optimizations (%(choices)s; default: %(default)s)')
    parser.add_argument('--mem-heap', metavar='SIZE', action='store', type=int, default=512, help='size of memory heap, in kilobytes (default: %(default)s)')
    parser.add_argument('--port-dir', metavar='DIR', action='store', default=DEFAULT_PORT_DIR, help='add port directory (default: %(default)s)')
    parser.add_argument('--profile', metavar='FILE', action='store', default=DEFAULT_PROFILE, help='specify profile file (default: %(default)s)')
    parser.add_argument('--snapshot-exec', metavar='X', choices=['ON', 'OFF'], default='OFF', type=str.upper, help='enable executing snapshot files (%(choices)s; default: %(default)s)')
    parser.add_argument('--snapshot-save', metavar='X', choices=['ON', 'OFF'], default='OFF', type=str.upper, help='enable saving snapshot files (%(choices)s; default: %(default)s)')
    parser.add_argument('--system-allocator', metavar='X', choices=['ON', 'OFF'], default='OFF', type=str.upper, help='enable system allocator (%(choices)s; default: %(default)s)')
    parser.add_argument('--static-link', metavar='X', choices=['ON', 'OFF'], default='ON', type=str.upper, help='enable static linking of binaries (%(choices)s; default: %(default)s)')
    parser.add_argument('--strip', metavar='X', choices=['ON', 'OFF'], default='ON', type=str.upper, help='strip release binaries (%(choices)s; default: %(default)s)')
    parser.add_argument('--toolchain', metavar='FILE', action='store', default=default_toolchain(), help='add toolchain file (default: %(default)s)')
    parser.add_argument('--unittests', action='store_const', const='ON', default='OFF', help='build unittests')
    parser.add_argument('-v', '--verbose', action='store_const', const='ON', default='OFF', help='increase verbosity')

    devgroup = parser.add_argument_group('developer options')
    devgroup.add_argument('--link-map', metavar='X', choices=['ON', 'OFF'], default='OFF', type=str.upper, help=devhelp('enable the generation of a link map file for jerry command line tool (%(choices)s; default: %(default)s)'))
    devgroup.add_argument('--mem-stats', metavar='X', choices=['ON', 'OFF'], default='OFF', type=str.upper, help=devhelp('enable memory statistics (%(choices)s; default: %(default)s)'))
    devgroup.add_argument('--mem-stress-test', metavar='X', choices=['ON', 'OFF'], default='OFF', type=str.upper, help=devhelp('enable mem-stress test (%(choices)s; default: %(default)s)'))
    devgroup.add_argument('--show-opcodes', metavar='X', choices=['ON', 'OFF'], default='OFF', type=str.upper, help=devhelp('enable parser byte-code dumps (%(choices)s; default: %(default)s)'))
    devgroup.add_argument('--show-regexp-opcodes', metavar='X', choices=['ON', 'OFF'], default='OFF', type=str.upper, help=devhelp('enable regexp byte-code dumps (%(choices)s; default: %(default)s)'))
    devgroup.add_argument('--valgrind', metavar='X', choices=['ON', 'OFF'], default='OFF', type=str.upper, help=devhelp('enable Valgrind support (%(choices)s; default: %(default)s)'))
    devgroup.add_argument('--valgrind-freya', metavar='X', choices=['ON', 'OFF'], default='OFF', type=str.upper, help=devhelp('enable Valgrind-Freya support (%(choices)s; default: %(default)s)'))

    arguments = parser.parse_args(args)
    if arguments.devhelp:
        parser.print_help()
        sys.exit(0)

    return arguments

def generate_build_options(arguments):
    build_options = []

    build_options.append('-DENABLE_ALL_IN_ONE=%s' % arguments.all_in_one)
    build_options.append('-DCMAKE_BUILD_TYPE=%s' % arguments.build_type)
    build_options.extend(arguments.cmake_param)
    build_options.append('-DEXTERNAL_COMPILE_FLAGS=' + ' '.join(arguments.compile_flag))
    build_options.append('-DFEATURE_CPOINTER_32_BIT=%s' % arguments.cpointer_32bit)
    build_options.append('-DFEATURE_ERROR_MESSAGES=%s' % arguments.error_messages)
    build_options.append('-DJERRY_CMDLINE=%s' % arguments.jerry_cmdline)
    build_options.append('-DJERRY_CMDLINE_MINIMAL=%s' % arguments.jerry_cmdline_minimal)
    build_options.append('-DJERRY_LIBC=%s' % arguments.jerry_libc)
    build_options.append('-DJERRY_LIBM=%s' % arguments.jerry_libm)
    build_options.append('-DFEATURE_JS_PARSER=%s' % arguments.js_parser)
    build_options.append('-DEXTERNAL_LINK_LIBS=' + ' '.join(arguments.link_lib))
    build_options.append('-DEXTERNAL_LINKER_FLAGS=' + ' '.join(arguments.linker_flag))
    build_options.append('-DENABLE_LTO=%s' % arguments.lto)
    build_options.append('-DMEM_HEAP_SIZE_KB=%d' % arguments.mem_heap)
    build_options.append('-DPORT_DIR=%s' % arguments.port_dir)

    if path.isabs(arguments.profile):
        PROFILE = arguments.profile
    else:
        PROFILE = path.join(PROFILE_DIR, arguments.profile + '.profile')

    build_options.append('-DFEATURE_PROFILE=%s' % PROFILE)

    build_options.append('-DFEATURE_DEBUGGER=%s' % arguments.jerry_debugger)
    build_options.append('-DFEATURE_SNAPSHOT_EXEC=%s' % arguments.snapshot_exec)
    build_options.append('-DFEATURE_SNAPSHOT_SAVE=%s' % arguments.snapshot_save)
    build_options.append('-DFEATURE_SYSTEM_ALLOCATOR=%s' % arguments.system_allocator)
    build_options.append('-DENABLE_STATIC_LINK=%s' % arguments.static_link)
    build_options.append('-DENABLE_STRIP=%s' % arguments.strip)

    if arguments.toolchain:
      build_options.append('-DCMAKE_TOOLCHAIN_FILE=%s' % arguments.toolchain)

    build_options.append('-DUNITTESTS=%s' % arguments.unittests)
    build_options.append('-DCMAKE_VERBOSE_MAKEFILE=%s' % arguments.verbose)

    # developer options
    build_options.append('-DENABLE_LINK_MAP=%s' % arguments.link_map)
    build_options.append('-DFEATURE_MEM_STATS=%s' % arguments.mem_stats)
    build_options.append('-DFEATURE_MEM_STRESS_TEST=%s' % arguments.mem_stress_test)
    build_options.append('-DFEATURE_PARSER_DUMP=%s' % arguments.show_opcodes)
    build_options.append('-DFEATURE_REGEXP_DUMP=%s' % arguments.show_regexp_opcodes)
    build_options.append('-DFEATURE_VALGRIND=%s' % arguments.valgrind)
    build_options.append('-DFEATURE_VALGRIND_FREYA=%s' % arguments.valgrind_freya)

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
