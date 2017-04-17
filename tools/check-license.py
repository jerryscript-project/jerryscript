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
import re
import sys
import settings

LICENSE = re.compile(
    r'((#|//|\*) Copyright .*\n'
    r')+\s?\2\n'
    r'\s?\2 Licensed under the Apache License, Version 2.0 \(the "License"\);\n'
    r'\s?\2 you may not use this file except in compliance with the License.\n'
    r'\s?\2 You may obtain a copy of the License at\n'
    r'\s?\2\n'
    r'\s?\2     http://www.apache.org/licenses/LICENSE-2.0\n'
    r'\s?\2\n'
    r'\s?\2 Unless required by applicable law or agreed to in writing, software\n'
    r'\s?\2 distributed under the License is distributed on an "AS IS" BASIS\n'
    r'\s?\2 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n'
    r'\s?\2 See the License for the specific language governing permissions and\n'
    r'\s?\2 limitations under the License.\n'
)

INCLUDE_DIRS = [
    'cmake',
    'jerry-core',
    'jerry-ext',
    'jerry-libc',
    'jerry-libm',
    'jerry-main',
    'jerry-port',
    'targets',
    'tests',
    'tools',
]

EXCLUDE_DIRS = [
    'targets/esp8266',
    os.path.relpath(settings.TEST262_TEST_SUITE_DIR, settings.PROJECT_DIR),
]

EXTENSIONS = [
    '.c',
    '.cpp',
    '.h',
    '.S',
    '.js',
    '.py',
    '.sh',
    '.tcl',
    '.cmake',
]


def main():
    is_ok = True

    for dname in INCLUDE_DIRS:
        for root, _, files in os.walk(dname):
            if any(root.startswith(exclude) for exclude in EXCLUDE_DIRS):
                continue
            for fname in files:
                if any(fname.endswith(ext) for ext in EXTENSIONS):
                    fpath = os.path.join(root, fname)
                    with open(fpath) as curr_file:
                        if not LICENSE.search(curr_file.read()):
                            print('%s: incorrect license' % fpath)
                            is_ok = False

    if not is_ok:
        sys.exit(1)


if __name__ == '__main__':
    main()
