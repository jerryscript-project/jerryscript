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
from __future__ import print_function

import argparse
import logging
import os
import subprocess
import shutil


TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.dirname(TOOLS_DIR)
SRCMERGER = os.path.join(TOOLS_DIR, 'srcmerger.py')
JERRY_CORE = os.path.join(ROOT_DIR, 'jerry-core')
JERRY_PORT = os.path.join(ROOT_DIR, 'jerry-port', 'default')
JERRY_LIBM = os.path.join(ROOT_DIR, 'jerry-libm')


def run_commands(*cmds, **kwargs):
    log = logging.getLogger('sourcegenerator')
    verbose = kwargs.get('verbose', False)

    for cmd in cmds:
        if verbose:
            cmd.append('--verbose')
        log.debug('Run command: %s', cmd)
        subprocess.call(cmd)


def generate_jerry_core(output_dir, verbose=False):
    cmd_jerry_c_gen = [
        'python', SRCMERGER,
        '--base-dir', JERRY_CORE,
        '--input={}/api/jerry.c'.format(JERRY_CORE),
        '--output={}/jerryscript.c'.format(output_dir),
        '--append-c-files',
        # Add the global built-in by default to include some common items
        # to avoid problems with common built-in headers
        '--input={}/ecma/builtin-objects/ecma-builtins.c'.format(JERRY_CORE),
        '--remove-include=jerryscript.h',
        '--remove-include=jerryscript-port.h',
        '--remove-include=jerryscript-compiler.h',
        '--remove-include=jerryscript-core.h',
        '--remove-include=jerryscript-debugger.h',
        '--remove-include=jerryscript-debugger-transport.h',
        '--remove-include=jerryscript-port.h',
        '--remove-include=jerryscript-snapshot.h',
        '--remove-include=config.h',
        '--push-include=jerryscript.h',
    ]

    cmd_jerry_h_gen = [
        'python', SRCMERGER,
        '--base-dir', JERRY_CORE,
        '--input={}/include/jerryscript.h'.format(JERRY_CORE),
        '--output={}/jerryscript.h'.format(output_dir),
        '--remove-include=config.h',
        '--push-include=jerryscript-config.h',
    ]

    run_commands(cmd_jerry_c_gen, cmd_jerry_h_gen, verbose=verbose)

    shutil.copyfile('{}/config.h'.format(JERRY_CORE),
                    '{}/jerryscript-config.h'.format(output_dir))


def generate_jerry_port_default(output_dir, verbose=False):
    cmd_port_c_gen = [
        'python', SRCMERGER,
        '--base-dir', JERRY_PORT,
        '--output={}/jerryscript-port-default.c'.format(output_dir),
        '--append-c-files',
        '--remove-include=jerryscript-port.h',
        '--remove-include=jerryscript-port-default.h',
        '--remove-include=jerryscript-debugger.h',
        '--push-include=jerryscript.h',
        '--push-include=jerryscript-port-default.h',
    ]

    cmd_port_h_gen = [
        'python', SRCMERGER,
        '--base-dir', JERRY_PORT,
        '--input={}/include/jerryscript-port-default.h'.format(JERRY_PORT),
        '--output={}/jerryscript-port-default.h'.format(output_dir),
        '--remove-include=jerryscript-port.h',
        '--remove-include=jerryscript.h',
        '--push-include=jerryscript.h',
    ]

    run_commands(cmd_port_c_gen, cmd_port_h_gen, verbose=verbose)


def generate_jerry_libm(output_dir, verbose=False):
    cmd_libm_c_gen = [
        'python', SRCMERGER,
        '--base-dir', JERRY_LIBM,
        '--output={}/jerryscript-libm.c'.format(output_dir),
        '--append-c-files',
    ]

    run_commands(cmd_libm_c_gen, verbose=verbose)

    shutil.copyfile('{}/include/math.h'.format(JERRY_LIBM),
                    '{}/math.h'.format(output_dir))

def main():
    parser = argparse.ArgumentParser(description='Generate single sources.')
    parser.add_argument('--jerry-core', action='store_true', dest='jerry_core',
                        help='Generate jerry-core files', default=False)
    parser.add_argument('--jerry-port-default', action='store_true', dest='jerry_port_default',
                        help='Generate jerry-port-default files', default=False)
    parser.add_argument('--jerry-libm', action='store_true', dest='jerry_libm',
                        help='Generate jerry-libm files', default=False)
    parser.add_argument('--output-dir', metavar='DIR', type=str, dest='output_dir',
                        default='gen_src', help='Output dir')
    parser.add_argument('--verbose', '-v', action='store_true', default=False)

    args = parser.parse_args()

    if args.verbose:
        logging.basicConfig(level=logging.DEBUG)

    try:
        os.makedirs(args.output_dir)
    except os.error:
        pass

    if args.jerry_core:
        generate_jerry_core(args.output_dir, args.verbose)

    if args.jerry_port_default:
        generate_jerry_port_default(args.output_dir, args.verbose)

    if args.jerry_libm:
        generate_jerry_libm(args.output_dir, args.verbose)


if __name__ == '__main__':
    main()
