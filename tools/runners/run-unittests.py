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
import glob
import os
import subprocess
import sys

TERM_NORMAL = '\033[0m'
TERM_RED = '\033[1;31m'
TERM_GREEN = '\033[1;32m'

def get_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument('-q', '--quiet', action='store_true',
                        help='Only print out failing tests')
    parser.add_argument('path',
                        help='Path of test binaries')

    script_args = parser.parse_args()
    return script_args


def get_unittests(path):
    unittests = []
    files = glob.glob(os.path.join(path, 'unit-*'))
    for testfile in files:
        if os.path.isfile(testfile) and os.access(testfile, os.X_OK):
            if sys.platform != 'win32' or testfile.endswith(".exe"):
                unittests.append(testfile)
    unittests.sort()
    return unittests


def main(args):
    unittests = get_unittests(args.path)
    total = len(unittests)
    if total == 0:
        print("%s: no unit-* test to execute", args.path)
        return 1

    tested = 0
    passed = 0
    failed = 0

    runtime = os.environ.get('RUNTIME')
    test_cmd = [runtime] if runtime else []

    for test in unittests:
        tested += 1
        testpath = os.path.relpath(test)
        try:
            subprocess.check_output(test_cmd + [test], stderr=subprocess.STDOUT, universal_newlines=True)
            passed += 1
            if not args.quiet:
                print("[%4d/%4d] %sPASS: %s%s" % (tested, total, TERM_GREEN, testpath, TERM_NORMAL))
        except subprocess.CalledProcessError as err:
            failed += 1
            print("[%4d/%4d] %sFAIL (%d): %s%s" % (tested, total, TERM_RED, err.returncode, testpath, TERM_NORMAL))
            print("================================================")
            print(err.output)
            print("================================================")

    print("\n[summary] %s\n" % os.path.join(os.path.relpath(args.path), "unit-*"))
    print("TOTAL: %d" % total)
    print("%sPASS: %d%s" % (TERM_GREEN, passed, TERM_NORMAL))
    print("%sFAIL: %d%s\n" % (TERM_RED, failed, TERM_NORMAL))

    success_color = TERM_GREEN if passed == total else TERM_RED
    print("%sSuccess: %d%%%s" % (success_color, passed*100/total, TERM_NORMAL))

    if failed > 0:
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main(get_arguments()))
