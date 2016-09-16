#!/usr/bin/env python

# Copyright 2016 Samsung Electronics Co., Ltd.
# Copyright 2016 University of Szeged.
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

from os import path

TOOLS_DIR = path.dirname(path.abspath(__file__))
PROJECT_DIR = path.normpath(path.join(TOOLS_DIR, '..'))
JERRY_TESTS_DIR = path.join(PROJECT_DIR, 'tests/jerry')
JERRY_TEST_SUITE_DIR = path.join(PROJECT_DIR, 'tests/jerry-test-suite')
JERRY_TEST_SUITE_MINIMAL_LIST = path.join(PROJECT_DIR, 'tests/jerry-test-suite/minimal-profile-list')

BUILD_SCRIPT = path.join(TOOLS_DIR, 'build.py')
CPPCHECK_SCRIPT = path.join(TOOLS_DIR, 'check-cppcheck.sh')
SIGNED_OFF_SCRIPT = path.join(TOOLS_DIR, 'check-signed-off.sh')
VERA_SCRIPT = path.join(TOOLS_DIR, 'check-vera.sh')
LICENSE_SCRIPT = path.join(TOOLS_DIR, 'check-license.py')
TEST_RUNNER_SCRIPT = path.join(TOOLS_DIR, 'runners/run-test-suite.sh')
UNITTEST_RUNNER_SCRIPT = path.join(TOOLS_DIR, 'runners/run-unittests.sh')
