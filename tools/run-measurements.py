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
import collections
import os
import subprocess
import sys
import settings


OUTPUT_DIR = os.path.join(settings.PROJECT_DIR, 'build', 'measurements')


def run(cmd, shell=False):
    sys.stderr.write('%s\n' % (' '.join(cmd) if isinstance(cmd, list) else cmd))
    return subprocess.check_output(cmd, shell=shell)


def compute_size(bin_path):
    lines = run(['size', '--format=berkeley', bin_path]).splitlines()
    sizes = lines[1].split()
    return (int(sizes[0]), int(sizes[1]))


def emoji(diff):
    negative = ':sunny:' #':white_check_mark:'
    zero = ':sunny:' #':heavy_check_mark:'
    positive = ':zap:' #':x:'

    return negative if diff < 0 else positive if diff > 0 else zero


def format_result(result, baseline=None):
    formatted_result = ' + '.join(['{:,}'.format(val) for val in result])
    if baseline:
        formatted_diff = ', '.join(['{:+,} {}'.format(val - bval, emoji(val - bval)) for val, bval in zip(result, baseline)])
        formatted_result += ' (' + formatted_diff + ')'
    return formatted_result


Variant = collections.namedtuple('Variant', ['name', 'build_args'])


VARIANTS = [
    Variant('arm-linux-minimal',
            ['--toolchain=cmake/toolchain_linux_armv7l.cmake', '--profile=minimal']),
    Variant('arm-linux-es5.1',
            ['--toolchain=cmake/toolchain_linux_armv7l.cmake', '--profile=es5.1']),
    Variant('arm-linux-es2015-subset',
            ['--toolchain=cmake/toolchain_linux_armv7l.cmake', '--profile=es2015-subset']),
]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('pathspec', metavar='PATHSPEC', default=['HEAD'], nargs='*',
                        help='Versions to measure (default: %(default)s)')
    parser.add_argument('--baseline', metavar='INDEX', type=int,
                        help='Index of the baseline pathspec (default: no baseline)')
    parser.add_argument('--outdir', metavar='DIR', default=OUTPUT_DIR,
                        help='Specify output directory (default: %(default)s)')
    args = parser.parse_args()

    hashes = dict()
    results = dict()

    for pathspec in args.pathspec:
        run(['git', 'checkout', pathspec])

        hashes[pathspec] =  run(['git', 'show', '--format=%h', '--no-patch']).strip()
        results[pathspec] = dict()

        for variant in VARIANTS:
            build_dir_path = os.path.join(args.outdir, hashes[pathspec], variant.name)
            run([settings.BUILD_SCRIPT] + variant.build_args + ['--builddir=%s' % build_dir_path])
            results[pathspec][variant.name] = compute_size(os.path.join(build_dir_path, 'bin', 'jerry'))

    print('| jerry variant |', end='')
    for pathspec in args.pathspec:
        print(' %s @ %s (text + data) |' % (pathspec, hashes[pathspec]), end='')
    print()

    print('|---|', end='')
    for pathspec in args.pathspec:
        print('---:|', end='')
    print()

    for variant in VARIANTS:
        print('| %s |' % variant.name, end='')

        baseline = results[args.pathspec[args.baseline]][variant.name] if args.baseline is not None else None

        for i, pathspec in enumerate(args.pathspec):
            print(' %s |' % format_result(results[pathspec][variant.name],
                                          baseline if args.baseline != i else None),
                  end='')
        print()


if __name__ == "__main__":
    main()
