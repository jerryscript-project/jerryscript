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
import os
import re
import shutil
import subprocess
import sys

import util

def get_platform_cmd_prefix():
    if sys.platform == 'win32':
        return ['cmd', '/S', '/C']
    return ['python2']  # The official test262.py isn't python3 compatible, but has python shebang.


def get_arguments():
    execution_runtime = os.environ.get('RUNTIME', '')
    parser = argparse.ArgumentParser()
    parser.add_argument('--runtime', metavar='FILE', default=execution_runtime,
                        help='Execution runtime (e.g. qemu)')
    parser.add_argument('--engine', metavar='FILE', required=True,
                        help='JerryScript binary to run tests with')
    parser.add_argument('--test-dir', metavar='DIR', required=True,
                        help='Directory contains test262 test suite')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--es51', action='store_true',
                       help='Run test262 ES5.1 version')
    group.add_argument('--es2015', default=False, const='default',
                       nargs='?', choices=['default', 'all', 'update'],
                       help='Run test262 - ES2015. default: all tests except excludelist, ' +
                       'all: all tests, update: all tests and update excludelist')

    args = parser.parse_args()

    if args.es2015:
        args.test_dir = os.path.join(args.test_dir, 'es2015')
    else:
        args.test_dir = os.path.join(args.test_dir, 'es51')

    return args


def prepare_test262_test_suite(args):
    if os.path.isdir(os.path.join(args.test_dir, '.git')):
        return 0

    return_code = subprocess.call(['git', 'clone', '--no-checkout',
                                   'https://github.com/tc39/test262.git', args.test_dir])
    if return_code:
        print('Cloning test262 repository failed.')
        return return_code

    if args.es2015:
        git_hash = 'fd44cd73dfbce0b515a2474b7cd505d6176a9eb5'
    else:
        git_hash = 'es5-tests'

    return_code = subprocess.call(['git', 'checkout', git_hash], cwd=args.test_dir)
    if return_code:
        print('Cloning test262 repository failed - invalid git revision.')
        return return_code

    if args.es2015:
        return_code = subprocess.call(['git', 'apply', os.path.join('..', '..', 'test262-es6.patch')],
                                      cwd=args.test_dir)
        if return_code:
            print('Applying test262-es6.patch failed')
            return return_code
    else:
        path_to_remove = os.path.join(args.test_dir, 'test', 'suite', 'bestPractice')
        if os.path.isdir(path_to_remove):
            shutil.rmtree(path_to_remove)

        path_to_remove = os.path.join(args.test_dir, 'test', 'suite', 'intl402')
        if os.path.isdir(path_to_remove):
            shutil.rmtree(path_to_remove)

    return 0


def prepare_exclude_list(args):
    if args.es2015 == 'all' or args.es2015 == 'update':
        return_code = subprocess.call(['git', 'checkout', 'excludelist.xml'], cwd=args.test_dir)
        if return_code:
            print('Reverting excludelist.xml failed')
            return return_code
    elif args.es2015 == 'default':
        shutil.copyfile(os.path.join('tests', 'test262-es6-excludelist.xml'),
                        os.path.join(args.test_dir, 'excludelist.xml'))

    return 0


def update_exclude_list(args):
    assert args.es2015 == 'update', "Only --es2015 option supports updating excludelist"

    print("=== Summary - updating excludelist ===\n")
    failing_tests = set()
    new_passing_tests = set()
    with open(os.path.join(os.path.dirname(args.engine), 'test262.report'), 'r') as report_file:
        summary_found = False
        for line in report_file:
            if summary_found:
                match = re.match(r"  (\S*) in ", line)
                if match:
                    failing_tests.add(match.group(1) + '.js')
            elif line.startswith('Failed Tests'):
                summary_found = True

    with open(os.path.join('tests', 'test262-es6-excludelist.xml'), 'r+') as exclude_file:
        lines = exclude_file.readlines()
        exclude_file.seek(0)
        exclude_file.truncate()

        # Skip the last line "</excludeList>" to be able to insert new failing tests.
        for line in lines[:-1]:
            match = re.match(r"  <test id=\"(\S*)\">", line)
            if match:
                test = match.group(1)
                if test in failing_tests:
                    failing_tests.remove(test)
                    exclude_file.write(line)
                else:
                    new_passing_tests.add(test)
            else:
                exclude_file.write(line)

        if failing_tests:
            print("New failing tests added to the excludelist")
            for test in sorted(failing_tests):
                exclude_file.write('  <test id="' + test + '"><reason></reason></test>\n')
                print("  " + test)
            print("")

        exclude_file.write('</excludeList>\n')

    if new_passing_tests:
        print("New passing tests removed from the excludelist")
        for test in sorted(new_passing_tests):
            print("  " + test)
        print("")

    if failing_tests or new_passing_tests:
        print("Excludelist was updated succesfully.")
        return 1

    print("Excludelist was already up-to-date.")
    return 0


def main(args):
    return_code = prepare_test262_test_suite(args)
    if return_code:
        return return_code

    return_code = prepare_exclude_list(args)
    if return_code:
        return return_code

    if sys.platform == 'win32':
        original_timezone = util.get_timezone()
        util.set_sighdl_to_reset_timezone(original_timezone)
        util.set_timezone('Pacific Standard Time')

    command = (args.runtime + ' ' + args.engine).strip()
    if args.es2015:
        try:
            subprocess.check_output(["timeout", "--version"])
            command = "timeout 3 " + command
        except subprocess.CalledProcessError:
            pass

    kwargs = {}
    if sys.version_info.major >= 3:
        kwargs['errors'] = 'ignore'
    proc = subprocess.Popen(get_platform_cmd_prefix() +
                            [os.path.join(args.test_dir, 'tools/packaging/test262.py'),
                             '--command', command,
                             '--tests', args.test_dir,
                             '--full-summary'],
                            universal_newlines=True,
                            stdout=subprocess.PIPE,
                            **kwargs)

    return_code = 1
    with open(os.path.join(os.path.dirname(args.engine), 'test262.report'), 'w') as output_file:
        counter = 0
        summary_found = False
        while True:
            counter += 1
            output = proc.stdout.readline()
            if not output:
                break
            output_file.write(output)
            if not summary_found and (counter % 100) == 0:
                print("\rExecuted approx %d tests..." % counter, end='')

            if output.startswith('=== Summary ==='):
                summary_found = True
                print('')

            if summary_found:
                print(output, end='')
                if 'All tests succeeded' in output:
                    return_code = 0

    proc.wait()

    if sys.platform == 'win32':
        util.set_timezone(original_timezone)

    if args.es2015 == 'update':
        return_code = update_exclude_list(args)

    return return_code


if __name__ == "__main__":
    sys.exit(main(get_arguments()))
