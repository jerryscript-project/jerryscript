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

try:
    from configparser import ConfigParser
except ImportError:
    from ConfigParser import ConfigParser

import argparse
import fileinput
import json
import os
import re
import subprocess
import sys

from settings import FORMAT_SCRIPT, PROJECT_DIR
from gen_c_source import LICENSE

MAGIC_STRINGS_INI = os.path.join(PROJECT_DIR, 'jerry-core', 'lit', 'lit-magic-strings.ini')
MAGIC_STRINGS_INC_H = os.path.join(PROJECT_DIR, 'jerry-core', 'lit', 'lit-magic-strings.inc.h')

ECMA_ERRORS_INI = os.path.join(PROJECT_DIR, 'jerry-core', 'ecma', 'base', 'ecma-error-messages.ini')
ECMA_ERRORS_INC_H = os.path.join(PROJECT_DIR, 'jerry-core', 'ecma', 'base', 'ecma-error-messages.inc.h')

PARSER_ERRORS_INI = os.path.join(PROJECT_DIR, 'jerry-core', 'parser', 'js', 'parser-error-messages.ini')
PARSER_ERRORS_INC_H = os.path.join(PROJECT_DIR, 'jerry-core', 'parser', 'js', 'parser-error-messages.inc.h')

LIMIT_MAGIC_STR_LENGTH = 255


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


def read_magic_string_defs(debug, ini_path, item_name):
    # Read the `jerry-core/lit/lit-magic-strings.ini` file and returns the magic
    # string definitions found therein in the form of
    #   [LIT_MAGIC_STRINGS]
    #   LIT_MAGIC_STRING_xxx = "vvv"
    #   ...
    # as
    #   [('LIT_MAGIC_STRING_xxx', 'vvv'), ...]
    # sorted by length and alpha.
    ini_parser = ConfigParser()
    ini_parser.optionxform = str  # case sensitive options (magic string IDs)
    ini_parser.read(ini_path)

    defs = [(str_ref, json.loads(str_value) if str_value != '' else '')
            for str_ref, str_value in ini_parser.items(item_name)]
    defs = sorted(defs, key=lambda ref_value: (len(ref_value[1]), ref_value[1]))

    if len(defs[-1][1]) > LIMIT_MAGIC_STR_LENGTH:
        for str_ref, str_value in [x for x in defs if len(x[1]) > LIMIT_MAGIC_STR_LENGTH]:
            print(f"error: The maximum allowed magic string size is "
                  f"{LIMIT_MAGIC_STR_LENGTH} but {str_ref} is {len(str_value)} long.")
        sys.exit(1)

    if debug:
        print(f'debug: magic string definitions: {debug_dump(defs)}')

    return defs


def extract_magic_string_refs(debug, pattern, inc_h_filename):
    results = {}

    def process_line(fname, lnum, line, guard_stack, pattern):
        # Build `results` dictionary as
        #   results['LIT_MAGIC_STRING_xxx'][('!defined (CONFIG_DISABLE_yyy_BUILTIN)', ...)]
        #       = [('zzz.c', 123), ...]
        # meaning that the given literal is referenced under the given guards at
        # the listed (file, line number) locations.
        exception_list = [f'{pattern}_DEF',
                          f'{pattern}_FIRST_STRING_WITH_SIZE',
                          f'{pattern}_LENGTH_LIMIT',
                          f'{pattern}__COUNT']

        for str_ref in re.findall(f'{pattern}_[a-zA-Z0-9_]+', line):
            if str_ref in exception_list:
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

    def process_file(fname, pattern):
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
                guards[-1] = f'!({guards[-1].strip()})'
                guards.append(process_guard(elif_match.group(1)))
            elif else_match is not None:
                guards = guard_stack[-1]
                guards[-1] = f'!({guards[-1].strip()})'
            elif endif_match is not None:
                guard_stack.pop()

            lnum = fileinput.filelineno()
            process_line(fname, lnum, line, guard_stack, pattern)

        if guard_stack:
            print(f'warning: {fname}: unbalanced preprocessor conditional '
                  f'directives (analysis finished with no closing `#endif` '
                  f'for {guard_stack})')

    for root, _, files in os.walk(os.path.join(PROJECT_DIR, 'jerry-core')):
        for fname in files:
            if (fname.endswith('.c') or fname.endswith('.h')) \
                    and fname != inc_h_filename:
                process_file(os.path.join(root, fname), pattern)

    if debug:
        print(f'debug: magic string references: {debug_dump(results)}')

    return results


def calculate_magic_string_guards(defs, uses, debug=False):
    extended_defs = []

    for str_ref, str_value in defs:
        if str_ref not in uses:
            print(f'warning: unused magic string {str_ref}')
            continue

        # Calculate the most compact guard, i.e., if a magic string is
        # referenced under various guards, keep the one that is more generic.
        # E.g.,
        # guard1 = A and B and C and D and E and F
        # guard2 = A and B and C
        # then guard1 or guard2 == guard2.
        guards = [set(guard_tuple) for guard_tuple in sorted(uses[str_ref].keys())]
        for i, guard_i in enumerate(guards):
            if guard_i is None:
                continue
            for guard in guard_i.copy():
                neg_guard = "!(" + guard[1:] + ")"
                for guard_j in guards:
                    if guard_j is not None and neg_guard in guard_j:
                        guard_i.remove(guard)
            for j, guard_j in enumerate(guards):
                if j == i or guard_j is None:
                    continue
                if guard_i < guard_j:
                    guards[j] = None
        guards = {tuple(sorted(guard)) for guard in guards if guard is not None}

        extended_defs.append((str_ref, str_value, guards))

    if debug:
        print(f'debug: magic string definitions (with guards): {debug_dump(extended_defs)}')

    return extended_defs


def guards_to_str(guards):
    return ' \\\n|| '.join(' && '.join(g.strip() for g in sorted(guard))
                           for guard in sorted(guards))


def generate_header(gen_file, ini_path):
    header = f"""{LICENSE}

/* This file is automatically generated by the {os.path.basename(__file__)} script
 * from {os.path.basename(ini_path)}. Do not edit! */
"""
    print(header, file=gen_file)


def generate_magic_string_defs(gen_file, defs, def_macro):
    last_guards = set([()])
    for str_ref, str_value, guards in defs:
        if last_guards != guards:
            if () not in last_guards:
                print(f'#endif /* {guards_to_str(last_guards)} */', file=gen_file)
            if () not in guards:
                print(f'#if {guards_to_str(guards)}', file=gen_file)

        print(f'{def_macro} ({str_ref}, {json.dumps(str_value)})', file=gen_file)

        last_guards = guards

    if () not in last_guards:
        print(f'#endif /* {guards_to_str(last_guards)} */', file=gen_file)


def generate_first_magic_strings(gen_file, defs, with_size_macro):
    print(file=gen_file)  # empty line separator

    max_size = len(defs[-1][1])
    for size in range(max_size + 1):
        last_guards = set([()])
        for str_ref, str_value, guards in defs:
            if len(str_value) >= size:
                if () not in guards and () in last_guards:
                    print(f'#if {guards_to_str(guards)}', file=gen_file)
                elif () not in guards and () not in last_guards:
                    if guards == last_guards:
                        continue
                    print(f'#elif {guards_to_str(guards)}', file=gen_file)
                elif () in guards and () not in last_guards:
                    print(f'#else /* !({guards_to_str(last_guards)}) */', file=gen_file)

                print(f'{with_size_macro} ({size}, {str_ref})', file=gen_file)

                if () in guards:
                    break

                last_guards = guards

        if () not in last_guards:
            print(f'#endif /* {guards_to_str(last_guards)} */', file=gen_file)


def generate_magic_strings(args, ini_path, item_name, pattern, inc_h_path, def_macro, with_size_macro=None):
    defs = read_magic_string_defs(args.debug, ini_path, item_name)
    uses = extract_magic_string_refs(args.debug, pattern, os.path.basename(inc_h_path))

    extended_defs = calculate_magic_string_guards(defs, uses, debug=args.debug)

    with open(inc_h_path, 'w', encoding='utf8') as gen_file:
        generate_header(gen_file, ini_path)
        generate_magic_string_defs(gen_file, extended_defs, def_macro)
        if with_size_macro:
            generate_first_magic_strings(gen_file, extended_defs, with_size_macro)


def main():
    parser = argparse.ArgumentParser(description='lit-magic-strings.inc.h generator')
    parser.add_argument('--debug', action='store_true', help='enable debug output')
    args = parser.parse_args()

    generate_magic_strings(args,
                           MAGIC_STRINGS_INI,
                           'LIT_MAGIC_STRINGS',
                           'LIT_MAGIC_STRING',
                           MAGIC_STRINGS_INC_H,
                           'LIT_MAGIC_STRING_DEF',
                           'LIT_MAGIC_STRING_FIRST_STRING_WITH_SIZE')

    generate_magic_strings(args,
                           ECMA_ERRORS_INI,
                           'ECMA_ERROR_MESSAGES',
                           'ECMA_ERR',
                           ECMA_ERRORS_INC_H,
                           'ECMA_ERROR_DEF')

    generate_magic_strings(args,
                           PARSER_ERRORS_INI,
                           'PARSER_ERR_MSG',
                           'PARSER_ERR',
                           PARSER_ERRORS_INC_H,
                           'PARSER_ERROR_DEF')

    subprocess.call([FORMAT_SCRIPT, '--fix'])


if __name__ == '__main__':
    main()
