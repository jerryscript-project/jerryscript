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

import argparse
import multiprocessing
import subprocess
from os import path
from glob import glob
import sys
import re

from settings import PROJECT_DIR

RE_CLANG_FORMAT_VERSION = re.compile(
    r".*clang-format version (?P<version>\d{2})\.")
RE_DIRECTIVE_COMMENT = re.compile(r"^#\s*(else|endif)$", re.MULTILINE)
RE_FUNCTION_NAME_COMMENT = re.compile(
    r"^\}(?!(?:\s\/\*\s\w+\s\*\/$|\s?\w*;))", re.MULTILINE)

CLANG_FORMAT_MIN_VERSION = 15

FOLDERS = ["jerry-core",
           "jerry-ext",
           "jerry-port",
           "jerry-math",
           "jerry-main",
           "tests/unit-core",
           "tests/unit-ext",
           "tests/unit-math"]


def get_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument('--fix', action='store_true', dest='fix',
                        help='fix source code stlye')
    parser.add_argument('--clang-format', dest='clang_format', default=f'clang-format-{CLANG_FORMAT_MIN_VERSION}',
                        help='path to clang-format executable')

    script_args = parser.parse_args()
    return script_args


def check_clang_format(args, source_file_name):
    cmd = [args.clang_format]

    if args.fix:
        cmd.append('-i')
    else:
        cmd.extend(['--dry-run', '--Werror'])

    cmd.append(source_file_name)

    with subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE) as proc:
        _, error = proc.communicate()

        if proc.returncode == 0:
            return 0

        print(error.decode('utf8'))

    return 1


def check_comments(regexps, source_file_name):
    failed = 0

    with open(source_file_name, encoding='utf-8') as source_file:
        data = source_file.read()

        for regexp in regexps:
            for match in re.finditer(regexp, data):
                failed += 1
                print("Invalid/Missing comment in %s:%d" %
                      (source_file_name, data.count('\n', 0, match.start()) + 1))

    return 1 if failed else 0


def run_pass(pool, process, process_args):
    return sum(pool.starmap(process, process_args))


def check_clang_format_version(args):
    try:
        out = subprocess.check_output([args.clang_format, '--version'])
        match = RE_CLANG_FORMAT_VERSION.match(out.decode('utf8'))

        if match and int(match.group('version')) >= CLANG_FORMAT_MIN_VERSION:
            return 0
    except OSError:
        pass

    return 1


def main(args):
    if check_clang_format_version(args) != 0:
        print(f"clang-format >= {CLANG_FORMAT_MIN_VERSION} is not installed.")
        return 1

    with multiprocessing.Pool() as pool:
        failed = 0

        for folder in FOLDERS:
            # pylint: disable=unexpected-keyword-arg
            files = sum(([glob(path.join(PROJECT_DIR, folder, f"**/*.{e}"), recursive=True)
                          for e in ['c', 'h']]), [])

            failed += run_pass(pool, check_clang_format,
                               [(args, sourece_file) for sourece_file in files])
            failed += run_pass(pool, check_comments,
                               [([RE_DIRECTIVE_COMMENT, RE_FUNCTION_NAME_COMMENT], sourece_file) for sourece_file in
                                files])

        return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(get_arguments()))
