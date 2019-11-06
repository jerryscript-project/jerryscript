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
import signal
import subprocess
import sys

TERM_NORMAL = '\033[0m'
TERM_RED = '\033[1;31m'
TERM_GREEN = '\033[1;32m'


def set_timezone(timezone):
    assert sys.platform == 'win32', "set_timezone is Windows only function"
    subprocess.call(['cmd', '/S', '/C', 'tzutil', '/s', timezone])


def set_timezone_and_exit(timezone):
    assert sys.platform == 'win32', "set_timezone_and_exit is Windows only function"
    set_timezone(timezone)
    sys.exit(1)


def get_timezone():
    assert sys.platform == 'win32', "get_timezone is Windows only function"
    return subprocess.check_output(['cmd', '/S', '/C', 'tzutil', '/g'])


def set_sighdl_to_reset_timezone(timezone):
    assert sys.platform == 'win32', "install_signal_handler_to_restore_timezone is Windows only function"
    signal.signal(signal.SIGINT, lambda signal, frame: set_timezone_and_exit(timezone))


def print_test_summary(summary_string, total, passed, failed):
    print("\n[summary] %s\n" % summary_string)
    print("TOTAL: %d" % total)
    print("%sPASS: %d%s" % (TERM_GREEN, passed, TERM_NORMAL))
    print("%sFAIL: %d%s\n" % (TERM_RED, failed, TERM_NORMAL))

    success_color = TERM_GREEN if passed == total else TERM_RED
    print("%sSuccess: %d%%%s" % (success_color, passed*100/total, TERM_NORMAL))


def print_test_result(tested, total, is_passed, passed_string, test_path, is_snapshot_generation=None):
    if is_snapshot_generation is None:
        snapshot_string = ''
    elif is_snapshot_generation:
        snapshot_string = ' (generate snapshot)'
    else:
        snapshot_string = ' (execute snapshot)'

    color = TERM_GREEN if is_passed else TERM_RED
    print("[%4d/%4d] %s%s: %s%s%s" % (tested, total, color, passed_string, test_path, snapshot_string, TERM_NORMAL))
