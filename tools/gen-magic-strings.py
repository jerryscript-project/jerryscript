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

try:
    from configparser import ConfigParser
except ImportError:
    from ConfigParser import ConfigParser

import argparse
import fileinput
import json
import os
import re

from settings import PROJECT_DIR


MAGIC_STRINGS_INI = os.path.join(PROJECT_DIR, 'jerry-core', 'lit', 'lit-magic-strings.ini')
MAGIC_STRINGS_INC_H = os.path.join(PROJECT_DIR, 'jerry-core', 'lit', 'lit-magic-strings.inc.h')


def debug_dump(obj):
    def deepcopy(obj):
        if isinstance(obj, (list, tuple)):
            return [deepcopy(e) for e in obj]
        if isinstance(obj, set):
            return [repr(e) for e in obj]
        if isinstance(obj, dict):
            return {repr(k): deepcopy(e) for k, e in obj.items()}
        return obj
    return json.dumps(deepcopy(obj), indent=4)


def read_magic_string_defs(debug=False):
    # Read the `jerry-core/lit/lit-magic-strings.ini` file and returns the magic
    # string definitions found therein in the form of
    #   [LIT_MAGIC_STRINGS]
    #   LIT_MAGIC_STRING_xxx = "vvv"
    #   ...
    # as
    #   [('LIT_MAGIC_STRING_xxx', 'vvv'), ...]
    # sorted by length and alpha.
    ini_parser = ConfigParser()
    ini_parser.optionxform = str # case sensitive options (magic string IDs)
    ini_parser.read(MAGIC_STRINGS_INI)

    defs = [(str_ref, json.loads(str_value) if str_value != '' else '')
            for str_ref, str_value in ini_parser.items('LIT_MAGIC_STRINGS')]
    defs = sorted(defs, key=lambda ref_value: (len(ref_value[1]), ref_value[1]))

    if debug:
        print('debug: magic string definitions: {dump}'
              .format(dump=debug_dump(defs)))

    return defs


def extract_magic_string_refs(debug=False):
    results = {}

    def process_line(fname, lnum, line, guard_stack):
        # Build `results` dictionary as
        #   results['LIT_MAGIC_STRING_xxx'][('!defined (CONFIG_DISABLE_yyy_BUILTIN)', ...)]
        #       = [('zzz.c', 123), ...]
        # meaning that the given literal is referenced under the given guards at
        # the listed (file, line number) locations.
        for str_ref in re.findall('LIT_MAGIC_STRING_[a-zA-Z0-9_]+', line):
            if str_ref in ['LIT_MAGIC_STRING_DEF',
                           'LIT_MAGIC_STRING_FIRST_STRING_WITH_SIZE',
                           'LIT_MAGIC_STRING_LENGTH_LIMIT',
                           'LIT_MAGIC_STRING__COUNT']:
                continue

            guard_set = set()
            for guards in guard_stack:
                guard_set.update(guards)
            guard_tuple = tuple(sorted(guard_set))

            if str_ref not in results:
                results[str_ref] = {}
            str_guards = results[str_ref]

            if guard_tuple not in str_guards:
                str_guards[guard_tuple] = []
            file_list = str_guards[guard_tuple]

            file_list.append((fname, lnum))

    def process_guard(guard):
        # Transform `#ifndef MACRO` to `#if !defined (MACRO)` and
        # `#ifdef MACRO` to `#if defined (MACRO)` to enable or-ing/and-ing the
        # conditions later on.
        if guard.startswith('ndef '):
            guard = guard.replace('ndef ', '!defined (', 1) + ')'
        elif guard.startswith('def '):
            guard = guard.replace('def ', 'defined (', 1) + ')'
        return guard

    def process_file(fname):
        # Builds `guard_stack` list for each line of a file as
        #   [['!defined (CONFIG_DISABLE_yyy_BUILTIN)', ...], ...]
        # meaning that all the listed guards (conditionals) have to hold for the
        # line to be kept by the preprocessor.
        guard_stack = []

        for line in fileinput.input(fname):
            if_match = re.match('^ *# *if(.*)', line)
            elif_match = re.match('^ *# *elif(.*)', line)
            else_match = re.match('^ *# *else', line)
            endif_match = re.match('^ *# *endif', line)
            if if_match is not None:
                guard_stack.append([process_guard(if_match.group(1))])
            elif elif_match is not None:
                guards = guard_stack[-1]
                guards[-1] = '!(%s)' % guards[-1]
                guards.append(process_guard(elif_match.group(1)))
            elif else_match is not None:
                guards = guard_stack[-1]
                guards[-1] = '!(%s)' % guards[-1]
            elif endif_match is not None:
                guard_stack.pop()

            lnum = fileinput.filelineno()
            process_line(fname, lnum, line, guard_stack)

        if guard_stack:
            print('warning: {fname}: unbalanced preprocessor conditional '
                  'directives (analysis finished with no closing `#endif` '
                  'for {guard_stack})'
                  .format(fname=fname, guard_stack=guard_stack))

    for root, _, files in os.walk(os.path.join(PROJECT_DIR, 'jerry-core')):
        for fname in files:
            if (fname.endswith('.c') or fname.endswith('.h')) \
               and fname != 'lit-magic-strings.inc.h':
                process_file(os.path.join(root, fname))

    if debug:
        print('debug: magic string references: {dump}'
              .format(dump=debug_dump(results)))

    return results


def calculate_magic_string_guards(defs, uses, debug=False):
    extended_defs = []

    for str_ref, str_value in defs:
        if str_ref not in uses:
            print('warning: unused magic string {str_ref}'
                  .format(str_ref=str_ref))
            continue

        # Calculate the most compact guard, i.e., if a magic string is
        # referenced under various guards, keep the one that is more generic.
        # E.g.,
        # guard1 = A and B and C and D and E and F
        # guard2 = A and B and C
        # then guard1 or guard2 == guard2.
        guards = [set(guard_tuple) for guard_tuple in uses[str_ref].keys()]
        for i, guard_i in enumerate(guards):
            if guard_i is None:
                continue
            for j, guard_j in enumerate(guards):
                if j == i or guard_j is None:
                    continue
                if guard_i < guard_j:
                    guards[j] = None
        guards = {tuple(sorted(guard)) for guard in guards if guard is not None}

        extended_defs.append((str_ref, str_value, guards))

    if debug:
        print('debug: magic string definitions (with guards): {dump}'
              .format(dump=debug_dump(extended_defs)))

    return extended_defs


def guards_to_str(guards):
    return ' \\\n|| '.join(' && '.join(g.strip() for g in sorted(guard))
                           for guard in sorted(guards))


def generate_header(gen_file):
    header = \
"""/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* This file is automatically generated by the %s script
 * from %s. Do not edit! */
""" % (os.path.basename(__file__), os.path.basename(MAGIC_STRINGS_INI))
    print(header, file=gen_file)


def generate_magic_string_defs(gen_file, defs):
    print(file=gen_file) # empty line separator

    last_guards = set([()])
    for str_ref, str_value, guards in defs:
        if last_guards != guards:
            if () not in last_guards:
                print('#endif', file=gen_file)
            if () not in guards:
                print('#if {guards}'.format(guards=guards_to_str(guards)), file=gen_file)

        print('LIT_MAGIC_STRING_DEF ({str_ref}, {str_value})'
              .format(str_ref=str_ref, str_value=json.dumps(str_value)), file=gen_file)

        last_guards = guards

    if () not in last_guards:
        print('#endif', file=gen_file)


def generate_first_magic_strings(gen_file, defs):
    print(file=gen_file) # empty line separator

    max_size = len(defs[-1][1])
    for size in range(max_size + 1):
        last_guards = set([()])
        for str_ref, str_value, guards in defs:
            if len(str_value) >= size:
                if () not in guards and () in last_guards:
                    print('#if {guards}'.format(guards=guards_to_str(guards)), file=gen_file)
                elif () not in guards and () not in last_guards:
                    if guards == last_guards:
                        continue
                    print('#elif {guards}'.format(guards=guards_to_str(guards)), file=gen_file)
                elif () in guards and () not in last_guards:
                    print('#else', file=gen_file)

                print('LIT_MAGIC_STRING_FIRST_STRING_WITH_SIZE ({size}, {str_ref})'
                      .format(size=size, str_ref=str_ref), file=gen_file)

                if () in guards:
                    break

                last_guards = guards

        if () not in last_guards:
            print('#endif', file=gen_file)


def main():
    parser = argparse.ArgumentParser(description='lit-magic-strings.inc.h generator')
    parser.add_argument('--debug', action='store_true', help='enable debug output')
    args = parser.parse_args()

    defs = read_magic_string_defs(debug=args.debug)
    uses = extract_magic_string_refs(debug=args.debug)

    extended_defs = calculate_magic_string_guards(defs, uses, debug=args.debug)

    with open(MAGIC_STRINGS_INC_H, 'w') as gen_file:
        generate_header(gen_file)
        generate_magic_string_defs(gen_file, extended_defs)
        generate_first_magic_strings(gen_file, extended_defs)


if __name__ == '__main__':
    main()
