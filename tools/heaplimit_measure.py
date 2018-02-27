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
# force // operator to be integer division in Python 2
from __future__ import division

import argparse
import json
import os
import subprocess
import sys

TOOLS_PATH = os.path.dirname(os.path.realpath(__file__))
BASE_PATH = os.path.join(TOOLS_PATH, '..')

FLAG_CLEAN = '--clean'
FLAG_DEBUG = '--debug'
FLAG_HEAPLIMIT = '--mem-heap'
JERRY_BUILDER = os.path.join(BASE_PATH, 'tools', 'build.py')
JERRY_BIN = os.path.join(BASE_PATH, 'build', 'bin', 'jerry')
TEST_DIR = os.path.join(BASE_PATH, 'tests')


def get_args():
    """ Parse input arguments. """
    desc = 'Finds the smallest possible JerryHeap size without failing to run the given js file'
    parser = argparse.ArgumentParser(description=desc)
    parser.add_argument('testfile')
    parser.add_argument('--heapsize', type=int, default=512,
                        help='set the limit of the first heapsize (default: %(default)d)')
    parser.add_argument('--buildtype', choices=['release', 'debug'], default='release',
                        help='select build type (default: %(default)s)')

    script_args = parser.parse_args()

    return script_args


def check_files(opts):
    files = [JERRY_BUILDER, opts.testfile]
    for _file in files:
        if not os.path.isfile(_file):
            sys.exit("File not found: %s" % _file)


def build_bin(heapsize, opts):
    """ Run tools/build.py script """
    command = [
        JERRY_BUILDER,
        FLAG_CLEAN,
        FLAG_HEAPLIMIT,
        str(heapsize)
    ]

    if opts.buildtype == 'debug':
        command.append(FLAG_DEBUG)

    print('Building JerryScript with: %s' % (' '.join(command)))
    subprocess.check_output(command)


def run_test(opts):
    """ Run the testfile to get the exitcode. """
    try:
        testfile = os.path.abspath(opts.testfile)
        run_cmd = [JERRY_BIN, testfile]
        # check output will raise an error if the exit code is not 0
        subprocess.check_output(run_cmd, cwd=TEST_DIR)
    except subprocess.CalledProcessError as err:
        return err.returncode

    return 0


def heap_limit(opts):
    """ Find the minimal size of jerryheap to pass """
    goodheap = opts.heapsize
    lowheap = 0
    hiheap = opts.heapsize

    while lowheap < hiheap:
        build_bin(hiheap, opts)
        assert os.path.isfile(JERRY_BIN), 'Jerry binary file does not exists'

        exitcode = run_test(opts)
        if exitcode != 0:
            lowheap = hiheap
            hiheap = (lowheap + goodheap) // 2
        else:
            goodheap = hiheap
            hiheap = (lowheap + hiheap) // 2

    return {
        'testfile': opts.testfile,
        'heaplimit to pass': goodheap
    }


def main(options):
    check_files(options)
    result = heap_limit(options)
    print(json.dumps(result, indent=4))


if __name__ == "__main__":
    main(get_args())
