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
import fileinput
import os
import re
import shlex
import sys


class DoctestExtractor(object):
    """
    An extractor to process Markdown files and find doctests inside.
    """

    def __init__(self, outdir, dry):
        """
        :param outdir: path to the directory where to write the found doctests.
        :param dry: if True, don't create the doctest files but print the file
            names only.
        """
        self._outdir = outdir
        self._dry = dry

        # Attributes actually initialized by process()
        self._infile = None
        self._outname_base = None
        self._outname_cnt = None

    def _warning(self, message, lineno):
        """
        Print a warning to the standard error.

        :param message: a description of the problem.
        :param lineno: the location that triggered the warning.
        """
        print('%s:%d: %s' % (self._infile, lineno, message), file=sys.stderr)

    def _process_decl(self, params):
        """
        Process a doctest declaration (`[doctest]: # (name="test.c", ...)`).

        :param params: the parameter string of the declaration (the string
            between the parentheses).
        :return: a tuple of a dictionary (of keys and values taken from the
            `params` string) and the line number of the declaration.
        """
        tokens = list(shlex.shlex(params))

        decl = {}
        for i in range(0, len(tokens), 4):
            if i + 2 >= len(tokens) or tokens[i + 1] != '=' or (i + 3 < len(tokens) and tokens[i + 3] != ','):
                self._warning('incorrect parameter list for test (key="value", ...)', fileinput.filelineno())
                decl = {}
                break
            decl[tokens[i]] = tokens[i + 2].strip('\'"')

        if 'name' not in decl:
            decl['name'] = '%s%d.c' % (self._outname_base, self._outname_cnt)
            self._outname_cnt += 1

        if 'test' not in decl:
            decl['test'] = 'run'

        return decl, fileinput.filelineno()

    def _process_code_start(self):
        """
        Process the beginning of a fenced code block (` ```c `).

        :return: a tuple of a list (of the first line(s) of the doctest) and the
            line number of the start of the code block.
        """
        return ['#line %d "%s"\n' % (fileinput.filelineno() + 1, self._infile)], fileinput.filelineno()

    def _process_code_end(self, decl, code):
        """
        Process the end of a fenced code block (` ``` `).

        :param decl: the dictionary of the declaration parameters.
        :param code: the list of lines of the doctest.
        """
        outname = os.path.join(self._outdir, decl['name']).replace('\\', '/')
        action = decl['test']
        if self._dry:
            print('%s %s' % (action, outname))
        else:
            with open(outname, 'w') as outfile:
                outfile.writelines(code)

    def process(self, infile):
        """
        Find doctests in a Markdown file and process them according to the
        constructor parameters.

        :param infile: path to the input file.
        """
        self._infile = infile
        self._outname_base = os.path.splitext(os.path.basename(infile))[0]
        self._outname_cnt = 1

        mode = 'TEXT'
        decl, decl_lineno = {}, 0
        code, code_lineno = [], 0

        for line in fileinput.input(infile):
            decl_match = re.match(r'^\[doctest\]:\s+#\s+\((.*)\)\s*$', line)
            nl_match = re.match(r'^\s*$', line)
            start_match = re.match(r'^```c\s*$', line)
            end_match = re.match(r'^```\s*', line)

            if mode == 'TEXT':
                if decl_match is not None:
                    decl, decl_lineno = self._process_decl(decl_match.group(1))
                    mode = 'NL'
            elif mode == 'NL':
                if decl_match is not None:
                    self._warning('test without code block', decl_lineno)
                    decl, decl_lineno = self._process_decl(decl_match.group(1))
                elif start_match is not None:
                    code, code_lineno = self._process_code_start()
                    mode = 'CODE'
                elif nl_match is None:
                    self._warning('test without code block', decl_lineno)
                    mode = 'TEXT'
            elif mode == 'CODE':
                if end_match is not None:
                    self._process_code_end(decl, code)
                    mode = 'TEXT'
                else:
                    code.append(line)

        if mode == 'NL':
            self._warning('test without code block', decl_lineno)
        elif mode == 'CODE':
            self._warning('unterminated code block', code_lineno)


def main():
    parser = argparse.ArgumentParser(description='Markdown doctest extractor', epilog="""
        The tool extracts specially marked fenced C code blocks from the input Markdown files
        and writes them to the file system. The annotations recognized by the tool are special
        but valid Markdown links/comments that must be added before the fenced code blocks:
        `[doctest]: # (name="test.c", ...)`. For now, two parameters are valid:
        `name` determines the filename for the extracted code block (overriding the default
        auto-numbered naming scheme), and `test` determines the test action to be performed on
        the extracted code (valid options are "compile", "link", and the default "run").
        """)
    parser.add_argument('-d', '--dir', metavar='NAME', default=os.getcwd(),
                        help='output directory name (default: %(default)s)')
    parser.add_argument('--dry', action='store_true',
                        help='don\'t generate files but print file names that would be generated '
                             'and what test action to perform on them')
    parser.add_argument('file', nargs='+',
                        help='input Markdown file(s)')
    args = parser.parse_args()

    extractor = DoctestExtractor(args.dir, args.dry)
    for mdfile in args.file:
        extractor.process(mdfile)


if __name__ == '__main__':
    main()
