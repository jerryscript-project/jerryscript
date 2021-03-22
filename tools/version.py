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
import settings


def main():
    parser = argparse.ArgumentParser(
        description='Display version of JerryScript',
        epilog="""
            Extract version information from sources without relying on
            compiler or preprocessor features.
            """
    )
    _ = parser.parse_args()

    with open(os.path.join(settings.PROJECT_DIR, 'jerry-core', 'include', 'jerryscript-types.h'), 'r') as header:
        version = {}
        version_re = re.compile(r'\s*#define\s+JERRY_API_(?P<key>MAJOR|MINOR|PATCH)_VERSION\s+(?P<value>\S+)')
        for line in header:
            match = version_re.match(line)
            if match:
                version[match.group('key')] = match.group('value')

    print('%(MAJOR)s.%(MINOR)s.%(PATCH)s' % version)


if __name__ == "__main__":
    main()
