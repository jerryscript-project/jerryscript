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
import os
import shutil
import subprocess
import sys

import util

def get_platform_cmd_prefix():
    if sys.platform == 'win32':
        return ['cmd', '/S', '/C']
    return ['python2']  # The official test262.py isn't python3 compatible, but has python shebang.


def run_test262_tests(runtime, engine, path_to_test262):
    if not os.path.isdir(os.path.join(path_to_test262, '.git')):
        return_code = subprocess.call(['git', 'clone', 'https://github.com/tc39/test262.git',
                                       '-b', 'es5-tests', path_to_test262])
        if return_code:
            print('Cloning test262 repository failed.')
            return return_code

    path_to_remove = os.path.join(path_to_test262, 'test', 'suite', 'bestPractice')
    if os.path.isdir(path_to_remove):
        shutil.rmtree(path_to_remove)

    path_to_remove = os.path.join(path_to_test262, 'test', 'suite', 'intl402')
    if os.path.isdir(path_to_remove):
        shutil.rmtree(path_to_remove)

    if sys.platform == 'win32':
        original_timezone = util.get_timezone()
        util.set_sighdl_to_reset_timezone(original_timezone)
        util.set_timezone('Pacific Standard Time')

    proc = subprocess.Popen(get_platform_cmd_prefix() +
                            [os.path.join(path_to_test262, 'tools/packaging/test262.py'),
                             '--command', (runtime + ' ' + engine).strip(),
                             '--tests', path_to_test262,
                             '--summary'],
                            universal_newlines=True,
                            stdout=subprocess.PIPE)

    return_code = 0
    with open(os.path.join(os.path.dirname(engine), 'test262.report'), 'w') as output_file:
        counter = 0
        summary_found = False
        while True:
            counter += 1
            output = proc.stdout.readline()
            if not output:
                break
            output_file.write(output)
            if (counter % 100) == 0:
                print("\rExecuted approx %d tests..." % counter, end='')

            if output.startswith('=== Summary ==='):
                summary_found = True
                print('')

            if summary_found:
                print(output, end='')
                if ('Failed tests' in output) or ('Expected to fail but passed' in output):
                    return_code = 1

    proc.wait()

    if sys.platform == 'win32':
        util.set_timezone(original_timezone)

    return return_code


def main():
    if len(sys.argv) != 3:
        print ("This script performs test262 compliance testing of the specified engine.")
        print ("")
        print ("Usage:")
        print ("  1st parameter: JavaScript engine to be tested.")
        print ("  2nd parameter: path to the directory with official test262 testsuite.")
        print ("")
        print ("Example:")
        print ("  ./run-test-suite-test262.py <engine> <test262_dir>")
        sys.exit(1)

    runtime = os.environ.get('RUNTIME', '')
    engine = sys.argv[1]
    path_to_test262 = sys.argv[2]

    sys.exit(run_test262_tests(runtime, engine, path_to_test262))

if __name__ == "__main__":
    main()
