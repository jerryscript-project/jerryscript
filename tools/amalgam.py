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
import fnmatch
import json
import logging
import os
import re
import shutil

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
JERRY_CORE = os.path.join(ROOT_DIR, 'jerry-core')
JERRY_PORT = os.path.join(ROOT_DIR, 'jerry-port', 'default')
JERRY_MATH = os.path.join(ROOT_DIR, 'jerry-math')


class Amalgamator(object):
    # pylint: disable=too-many-instance-attributes

    _RE_INCLUDE = re.compile(r'\s*#include ("|<)(.*?)("|>)\n$')

    def __init__(self, h_files, extra_includes=(), remove_includes=(), add_lineinfo=False):
        self._h_files = h_files
        self._extra_includes = extra_includes
        self._remove_includes = remove_includes
        self._add_lineinfo = add_lineinfo
        self._last_builtin = None
        self._processed = []
        self._output = []
        # The copyright will be loaded from the first input file
        self._copyright = {'lines': [], 'loaded': False}

    def _process_non_include(self, line, file_level):
        # Special case #2: Builtin include header name usage
        if line.strip() == "#include BUILTIN_INC_HEADER_NAME":
            assert self._last_builtin is not None, 'No previous BUILTIN_INC_HEADER_NAME definition'
            logging.debug('[%d] Detected usage of BUILTIN_INC_HEADER_NAME, including: "%s"',
                          file_level, self._last_builtin)
            self.add_file(self._h_files[self._last_builtin])
            # return from the function as we have processed the included file
            return

        # Special case #1: Builtin include header name definition
        if line.startswith('#define BUILTIN_INC_HEADER_NAME '):
            # the line is in this format: #define BUILTIN_INC_HEADER_NAME "<filename>"
            self._last_builtin = line.split('"', 2)[1]
            logging.debug('[%d] Detected definition of BUILTIN_INC_HEADER_NAME: "%s"', file_level, self._last_builtin)

        # the line is not anything special, just push it into the output
        self._output.append(line)

    def _emit_lineinfo(self, line_number, filename):
        if not self._add_lineinfo:
            return

        normalized_path = repr(os.path.normpath(filename))[1:-1]
        line_info = '#line %d "%s"\n' % (line_number, normalized_path)

        if self._output and self._output[-1].startswith('#line'):
            # Avoid emitting multiple line infos in sequence, just overwrite the last one
            self._output[-1] = line_info
        else:
            self._output.append(line_info)

    def add_file(self, filename, file_level=0):
        if os.path.basename(filename) in self._processed:
            logging.warning('Tried to to process an already processed file: "%s"', filename)
            return

        if not file_level:
            logging.debug('Adding file: "%s"', filename)

        file_level += 1

        # mark the start of the new file in the output
        self._emit_lineinfo(1, filename)

        line_idx = 0
        with open(filename, 'r') as input_file:
            in_copyright = False
            for line in input_file:
                line_idx += 1

                if not in_copyright and line.startswith('/* Copyright '):
                    in_copyright = True
                    if not self._copyright['loaded']:
                        self._copyright['lines'].append(line)
                    continue

                if in_copyright:
                    if not self._copyright['loaded']:
                        self._copyright['lines'].append(line)

                    if line.strip().endswith('*/'):
                        in_copyright = False
                        self._copyright['loaded'] = True
                        # emit a line info so the line numbering can be tracked correctly
                        self._emit_lineinfo(line_idx + 1, filename)

                    continue

                # check if the line is an '#include' line
                match = self._RE_INCLUDE.match(line)
                if not match:
                    # the line is not a header
                    self._process_non_include(line, file_level)
                    continue

                if match.group(1) == '<':
                    # found a "global" include
                    self._output.append(line)
                    continue

                name = match.group(2)

                if name in self._remove_includes:
                    logging.debug('[%d] Removing include line (%s:%d): %s',
                                  file_level, filename, line_idx, line.strip())
                    # emit a line info so the line numbering can be tracked correctly
                    self._emit_lineinfo(line_idx + 1, filename)
                    continue

                if name not in self._h_files:
                    logging.warning('[%d] Include not found (%s:%d): "%s"', file_level, filename, line_idx, name)
                    self._output.append(line)
                    continue

                if name in self._processed:
                    logging.debug('[%d] Already included: "%s"', file_level, name)
                    # emit a line info so the line numbering can be tracked correctly
                    self._emit_lineinfo(line_idx + 1, filename)
                    continue

                logging.debug('[%d] Including: "%s"', file_level, self._h_files[name])
                self.add_file(self._h_files[name], file_level)

                # mark the continuation of the current file in the output
                self._emit_lineinfo(line_idx + 1, filename)

                if not name.endswith('.inc.h'):
                    # if the included file is not a "*.inc.h" file mark it as processed
                    self._processed.append(name)

        file_level -= 1
        if not filename.endswith('.inc.h'):
            self._processed.append(os.path.basename(filename))

    def write_output(self, out_fp):
        for line in self._copyright['lines']:
            out_fp.write(line)

        for include in self._extra_includes:
            out_fp.write('#include "%s"\n' % include)

        for line in self._output:
            out_fp.write(line)


def match_files(base_dir, pattern):
    """
    Return the files matching the given pattern.

    :param base_dir: directory to search in
    :param pattern: file pattern to use
    :returns generator: the generator which iterates the matching file names
    """
    for path, _, files in os.walk(base_dir):
        for name in files:
            if fnmatch.fnmatch(name, pattern):
                yield os.path.join(path, name)


def collect_files(base_dir, pattern):
    """
    Collect files in the provided base directory given a file pattern.
    Will collect all files in the base dir recursively.

    :param base_dir: directory to search in
    :param pattern: file pattern to use
    :returns dictionary: a dictionary file base name -> file path mapping
    """
    name_mapping = {}
    for fname in match_files(base_dir, pattern):
        name = os.path.basename(fname)

        if name in name_mapping:
            logging.warning('Duplicate name detected: "%s" and "%s"', fname, name_mapping[name])
            continue

        name_mapping[name] = fname

    return name_mapping


def amalgamate(base_dir, input_files=(), output_file=None,
               append_c_files=False, remove_includes=(), extra_includes=(),
               add_lineinfo=False):
    """
    :param input_files: Main input source/header files
    :param output_file: Output source/header file
    :param append_c_files: Enable auto inclusion of c files under the base-dir
    :param add_lineinfo: Enable #line macro insertion into the generated sources
    """
    logging.debug('Starting merge with args: %s', json.dumps(locals(), indent=4, sort_keys=True))

    h_files = collect_files(base_dir, '*.h')
    c_files = collect_files(base_dir, '*.c')

    for name in remove_includes:
        c_files.pop(name, '')
        h_files.pop(name, '')

    amalgam = Amalgamator(h_files, extra_includes, remove_includes, add_lineinfo)
    for input_file in input_files:
        amalgam.add_file(input_file)

    if append_c_files:
        # if the input file is in the C files list it should be removed to avoid
        # double inclusion of the file
        for input_file in input_files:
            input_name = os.path.basename(input_file)
            c_files.pop(input_name, '')

        # Add the C files in reverse order to make sure that builtins are
        # not at the beginning.
        for fname in sorted(c_files.values(), reverse=True):
            amalgam.add_file(fname)

    with open(output_file, 'w') as output:
        amalgam.write_output(output)


def amalgamate_jerry_core(output_dir):
    amalgamate(
        base_dir=JERRY_CORE,
        input_files=[
            os.path.join(JERRY_CORE, 'api', 'jerry.c'),
            # Add the global built-in by default to include some common items
            # to avoid problems with common built-in headers
            os.path.join(JERRY_CORE, 'ecma', 'builtin-objects', 'ecma-builtins.c'),
        ],
        output_file=os.path.join(output_dir, 'jerryscript.c'),
        append_c_files=True,
        remove_includes=[
            'jerryscript.h',
            'jerryscript-port.h',
            'jerryscript-compiler.h',
            'jerryscript-core.h',
            'jerryscript-debugger.h',
            'jerryscript-debugger-transport.h',
            'jerryscript-port.h',
            'jerryscript-snapshot.h',
            'config.h',
        ],
        extra_includes=['jerryscript.h'],
    )

    amalgamate(
        base_dir=JERRY_CORE,
        input_files=[
            os.path.join(JERRY_CORE, 'include', 'jerryscript.h'),
            os.path.join(JERRY_CORE, 'include', 'jerryscript-debugger-transport.h'),
        ],
        output_file=os.path.join(output_dir, 'jerryscript.h'),
        remove_includes=['config.h'],
        extra_includes=['jerryscript-config.h'],
    )

    shutil.copyfile(os.path.join(JERRY_CORE, 'config.h'),
                    os.path.join(output_dir, 'jerryscript-config.h'))


def amalgamate_jerry_port_default(output_dir):
    amalgamate(
        base_dir=JERRY_PORT,
        output_file=os.path.join(output_dir, 'jerryscript-port-default.c'),
        append_c_files=True,
        remove_includes=[
            'jerryscript-port.h',
            'jerryscript-port-default.h',
            'jerryscript-debugger.h',
        ],
        extra_includes=[
            'jerryscript.h',
            'jerryscript-port-default.h',
        ],
    )

    amalgamate(
        base_dir=JERRY_PORT,
        input_files=[os.path.join(JERRY_PORT, 'include', 'jerryscript-port-default.h')],
        output_file=os.path.join(output_dir, 'jerryscript-port-default.h'),
        remove_includes=[
            'jerryscript-port.h',
            'jerryscript.h',
        ],
        extra_includes=['jerryscript.h'],
    )


def amalgamate_jerry_math(output_dir):
    amalgamate(
        base_dir=JERRY_MATH,
        output_file=os.path.join(output_dir, 'jerryscript-math.c'),
        append_c_files=True,
    )

    shutil.copyfile(os.path.join(JERRY_MATH, 'include', 'math.h'),
                    os.path.join(output_dir, 'math.h'))

def main():
    parser = argparse.ArgumentParser(description='Generate amalgamated sources.')
    parser.add_argument('--jerry-core', action='store_true',
                        help='amalgamate jerry-core files')
    parser.add_argument('--jerry-port-default', action='store_true',
                        help='amalgamate jerry-port-default files')
    parser.add_argument('--jerry-math', action='store_true',
                        help='amalgamate jerry-math files')
    parser.add_argument('--output-dir', metavar='DIR', default='amalgam',
                        help='output dir (default: %(default)s)')
    parser.add_argument('--verbose', '-v', action='store_true',
                        help='increase verbosity')

    args = parser.parse_args()

    if args.verbose:
        logging.basicConfig(level=logging.DEBUG)

    try:
        os.makedirs(args.output_dir)
    except os.error:
        pass

    if args.jerry_core:
        amalgamate_jerry_core(args.output_dir)

    if args.jerry_port_default:
        amalgamate_jerry_port_default(args.output_dir)

    if args.jerry_math:
        amalgamate_jerry_math(args.output_dir)


if __name__ == '__main__':
    main()
