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
    group.add_argument('--es2015', action='store_true',
                       help='Run test262 ES2015 version')

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
        shutil.copyfile(os.path.join('tests', 'test262-es6-excludelist.xml'),
                        os.path.join(args.test_dir, 'excludelist.xml'))

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

def main(args):
    return_code = prepare_test262_test_suite(args)
    if return_code:
        return return_code

    if sys.platform == 'win32':
        original_timezone = util.get_timezone()
        util.set_sighdl_to_reset_timezone(original_timezone)
        util.set_timezone('Pacific Standard Time')

    proc = subprocess.Popen(get_platform_cmd_prefix() +
                            [os.path.join(args.test_dir, 'tools/packaging/test262.py'),
                             '--command', (args.runtime + ' ' + args.engine).strip(),
                             '--tests', args.test_dir,
                             '--summary'],
                            universal_newlines=True,
                            stdout=subprocess.PIPE)

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

    return return_code


if __name__ == "__main__":
    sys.exit(main(get_arguments()))
