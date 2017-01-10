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
"""
Generate pins.js for a specified target, using target definitions from the
mbed OS source tree.

It's expecting to be run from the targets/mbedos5 directory.
"""

from __future__ import print_function

from pycparser import parse_file, c_ast, c_generator
from pycparserext.ext_c_parser import GnuCParser

from simpleeval import SimpleEval, DEFAULT_OPERATORS

import ast

import argparse
import json
import sys
import os

# import mbed tools
sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'mbed-os'))
from tools.targets import Target


def find_file(root_dir, directories, name):
    """
    Find the first instance of file with name 'name' in the directory tree
    starting with 'root_dir'.

    Filter out directories that are not in directories, or do not start with
    TARGET_.

    Since this looks in (essentially )the same directories as the compiler would
    when compiling mbed OS, we should only find one PinNames.h.
    """

    for root, dirs, files in os.walk(root_dir, topdown=True):
        # modify dirs in place
        dirs[:] = filter(lambda x: x in directories or not x.startswith('TARGET_'), dirs)

        if name in files:
            return os.path.join(root, name)

def enumerate_includes(root_dir, directories):
    """
    Walk through the directory tree, starting at root_dir, and enumerate all
    valid include directories.
    """
    for root, dirs, files in os.walk(root_dir, topdown=True):
        # modify dirs in place
        dirs[:] = filter(lambda x: x in directory_labels
                                or (    not x.startswith('TARGET_')
                                    and not x.startswith('TOOLCHAIN_')), dirs)
        yield root


class TypeDeclVisitor(c_ast.NodeVisitor):
    def __init__(self, filter_names=[]):
        self.names = filter_names

    def visit(self, node):
        value = None

        if node.__class__.__name__ == "TypeDecl":
            value = self.visit_TypeDecl(node)

        if value is None:
            for name, c in node.children():
                value = value or self.visit(c)

        return value

    def visit_TypeDecl(self, node):
        if node.declname in self.names:
            c_gen = c_generator.CGenerator()
            pins = {}

            operators = DEFAULT_OPERATORS
            operators[ast.BitOr] = lambda a, b: a | b
            operators[ast.LShift] = lambda a, b: a << b
            operators[ast.RShift] = lambda a, b: a << b
            evaluator = SimpleEval(DEFAULT_OPERATORS )

            for pin in node.type.values.enumerators:
                expr = c_gen.visit(pin.value)

                if "(int)" in expr:
                    expr = expr.replace('(int)', '')

                if expr in pins:
                    pins[pin.name] = pins[expr]
                else:
                    pins[pin.name] = evaluator.eval(expr.strip())

            return pins

def enumerate_pins(c_source_file, include_dirs, definitions):
    """
    Enumerate pins specified in PinNames.h, by looking for a PinName enum
    typedef somewhere in the file.
    """
    definitions += ['__attribute(x)__=', '__extension__(x)=', 'register=', '__IO=', 'uint32_t=unsigned int']

    gcc_args = ['-E', '-fmerge-all-constants']
    gcc_args += ['-I' + directory for directory in include_dirs]

    gcc_args += ['-D' + definition for definition in definitions]
    ast = parse_file(c_source_file,
                    use_cpp=True,
                    cpp_path='arm-none-eabi-gcc',
                    cpp_args=gcc_args,
                    parser=GnuCParser())

    # now, walk the AST
    v = TypeDeclVisitor(['PinName'])
    return v.visit(ast)


if __name__ == "__main__":
    if not os.path.exists('./mbed-os'):
        print("Fatal: mbed-os directory does not exist.")
        print("Try running 'make getlibs'")
        sys.exit(1)

    description = """
    Generate pins.js for a specified mbed board, using target definitions from the
    mbed OS source tree.
    """

    parser = argparse.ArgumentParser(description=description)

    parser.add_argument('board', help='mbed board name')
    parser.add_argument('-o',
                        help='Output JavaScript file (default: %(default)s)',
                        default='js/pins.js',
                        type=argparse.FileType('w'))
    parser.add_argument('-c',
                        help='Output C++ file (default: %(default)s)',
                        default='source/pins.cpp',
                        type=argparse.FileType('w'))

    args = parser.parse_args()
    board_name = args.board.upper()

    target = Target.get_target(board_name)

    directory_labels = ['TARGET_' + label for label in target.labels] + target.macros

    targets_dir = os.path.join('.', 'mbed-os', 'targets')

    pins_file = find_file(targets_dir, directory_labels, 'PinNames.h')

    includes = enumerate_includes(targets_dir, directory_labels)
    defines = list(directory_labels)

    # enumerate pins from PinNames.h
    pins = enumerate_pins(pins_file, ['./tools'] + list(includes), defines)

    # first sort alphabetically, then by length.
    pins = [ (x, pins[x]) for x in pins] # turn dict into tuples, which can be sorted
    pins = sorted(pins, key = lambda x: (len(x[0]), x[0].lower()))

    out_file = '\r\n'.join(['var %s = %s;' % pin for pin in pins])
    args.o.write(out_file)

    LICENSE = '''/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the \"License\");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an \"AS IS\" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This file is generated by generate_pins.py. Please do not modify.
 */
    '''

    COUNT = '''
unsigned int jsmbed_js_magic_string_count = {};
    '''.format(len(pins))

    LENGTHS = ',\n    '.join(str(len(pin[0])) for pin in pins)
    LENGTHS_SOURCE = '''
unsigned int jsmbed_js_magic_string_lengths[] = {
    %s
};
    ''' % LENGTHS

    MAGIC_VALUES = ',\n    '.join(str(pin[1]) for pin in pins)
    MAGIC_SOURCE = '''
unsigned int jsmbed_js_magic_string_values[] = {
    %s
};
    ''' % MAGIC_VALUES

    MAGIC_STRINGS = ',\n    '.join('"' + pin[0] + '"' for pin in pins)
    MAGIC_STRING_SOURCE = '''
const char * jsmbed_js_magic_strings[] = {
    %s
};
    ''' % MAGIC_STRINGS

    args.c.write(LICENSE + COUNT + LENGTHS_SOURCE + MAGIC_SOURCE + MAGIC_STRING_SOURCE)
