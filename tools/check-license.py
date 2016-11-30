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

import os
import re
import sys


license = re.compile(
u"""((#|//|\*) Copyright .*
)+\s?\\2
\s?\\2 Licensed under the Apache License, Version 2.0 \(the "License"\);
\s?\\2 you may not use this file except in compliance with the License.
\s?\\2 You may obtain a copy of the License at
\s?\\2
\s?\\2     http://www.apache.org/licenses/LICENSE-2.0
\s?\\2
\s?\\2 Unless required by applicable law or agreed to in writing, software
\s?\\2 distributed under the License is distributed on an "AS IS" BASIS
\s?\\2 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
\s?\\2 See the License for the specific language governing permissions and
\s?\\2 limitations under the License.""")

dirs = [
    'cmake',
    'jerry-core',
    'jerry-libc',
    'jerry-libm',
    'jerry-main',
    'targets',
    'tests',
    'tools',
]

exclude_dirs = [
    'targets/esp8266'
]

exts = [
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

    for dname in dirs:
        for root, _, files in os.walk(dname):
            if any(root.startswith(exclude) for exclude in exclude_dirs):
                continue
            for fname in files:
                if any(fname.endswith(ext) for ext in exts):
                    fpath = os.path.join(root, fname)
                    with open(fpath) as f:
                        if not license.search(f.read()):
                            print('%s: incorrect license' % fpath)
                            is_ok = False

    if not is_ok:
        sys.exit(1)


if __name__ == '__main__':
    main()
