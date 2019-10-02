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
import logging
import os
import re
import sys


class SourceMerger(object):
    # pylint: disable=too-many-instance-attributes

    _RE_INCLUDE = re.compile(r'\s*#include ("|<)(.*?)("|>)\n$')

    def __init__(self, h_files, extra_includes=None, remove_includes=None, add_lineinfo=False):
        self._log = logging.getLogger('sourcemerger')
        self._last_builtin = None
        self._processed = []
        self._output = []
        self._h_files = h_files
        self._extra_includes = extra_includes or []
        self._remove_includes = remove_includes
        self._add_lineinfo = add_lineinfo
        # The copyright will be loaded from the first input file
        self._copyright = {'lines': [], 'loaded': False}

    def _process_non_include(self, line, file_level):
        # Special case #2: Builtin include header name usage
        if line.strip() == "#include BUILTIN_INC_HEADER_NAME":
            assert self._last_builtin is not None, 'No previous BUILTIN_INC_HEADER_NAME definition'
            self._log.debug('[%d] Detected usage of BUILTIN_INC_HEADER_NAME, including: %s',
                            file_level, self._last_builtin)
            self.add_file(self._h_files[self._last_builtin])
            # return from the function as we have processed the included file
            return

        # Special case #1: Builtin include header name definition
        if line.startswith('#define BUILTIN_INC_HEADER_NAME '):
            # the line is in this format: #define BUILTIN_INC_HEADER_NAME "<filename>"
            self._last_builtin = line.split('"', 2)[1]
            self._log.debug('[%d] Detected definition of BUILTIN_INC_HEADER_NAME: %s',
                            file_level, self._last_builtin)

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
            self._log.warning('Tried to to process an already processed file: "%s"', filename)
            return

        if not file_level:
            self._log.debug('Adding file: "%s"', filename)

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
                match = SourceMerger._RE_INCLUDE.match(line)
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
                    self._log.debug('[%d] Removing include line (%s:%d): %s',
                                    file_level, filename, line_idx, line.strip())
                    # emit a line info so the line numbering can be tracked correctly
                    self._emit_lineinfo(line_idx + 1, filename)
                    continue

                if name not in self._h_files:
                    self._log.warning('[%d] Include not found: "%s" in "%s:%d"',
                                      file_level, name, filename, line_idx)
                    self._output.append(line)
                    continue

                if name in self._processed:
                    self._log.debug('[%d] Already included: "%s"',
                                    file_level, name)
                    # emit a line info so the line numbering can be tracked correctly
                    self._emit_lineinfo(line_idx + 1, filename)
                    continue

                self._log.debug('[%d] Including: "%s"',
                                file_level, self._h_files[name])
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
            print('Duplicate name detected: "%s" and "%s"' % (name, name_mapping[name]))
            continue

        name_mapping[name] = fname

    return name_mapping


def run_merger(args):
    h_files = collect_files(args.base_dir, '*.h')
    c_files = collect_files(args.base_dir, '*.c')

    for name in args.remove_include:
        c_files.pop(name, '')
        h_files.pop(name, '')

    merger = SourceMerger(h_files, args.push_include, args.remove_include, args.add_lineinfo)
    for input_file in args.input_files:
        merger.add_file(input_file)

    if args.append_c_files:
        # if the input file is in the C files list it should be removed to avoid
        # double inclusion of the file
        for input_file in args.input_files:
            input_name = os.path.basename(input_file)
            c_files.pop(input_name, '')

        # Add the C files in reverse order to make sure that builtins are
        # not at the beginning.
        for fname in sorted(c_files.values(), reverse=True):
            merger.add_file(fname)

    with open(args.output_file, 'w') as output:
        merger.write_output(output)


def main():
    parser = argparse.ArgumentParser(description='Merge source/header files.')
    parser.add_argument('--base-dir', metavar='DIR', type=str, dest='base_dir',
                        help='', default=os.path.curdir)
    parser.add_argument('--input', metavar='FILES', type=str, action='append', dest='input_files',
                        help='Main input source/header files', default=[])
    parser.add_argument('--output', metavar='FILE', type=str, dest='output_file',
                        help='Output source/header file')
    parser.add_argument('--append-c-files', dest='append_c_files', default=False,
                        action='store_true', help='Enable auto inclusion of c files under the base-dir')
    parser.add_argument('--remove-include', action='append', default=[])
    parser.add_argument('--push-include', action='append', default=[])
    parser.add_argument('--add-lineinfo', action='store_true', default=False,
                        help='Enable #line macro insertion into the generated sources')
    parser.add_argument('--verbose', '-v', action='store_true', default=False)

    args = parser.parse_args()

    log_level = logging.WARNING
    if args.verbose:
        log_level = logging.DEBUG

    logging.basicConfig(level=log_level)
    logging.debug('Starting merge with args: %s', str(sys.argv))

    run_merger(args)


if __name__ == "__main__":
    main()
