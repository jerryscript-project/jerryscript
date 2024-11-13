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

import codecs
import signal
import subprocess
import sys

if sys.platform == 'win32':
    TERM_NORMAL = ''
    TERM_RED = ''
    TERM_GREEN = ''
    TERM_YELLOW = ''
    TERM_BLUE = ''
else:
    TERM_NORMAL = '\033[0m'
    TERM_RED = '\033[1;31m'
    TERM_GREEN = '\033[1;32m'
    TERM_YELLOW = '\033[1;33m'
    TERM_BLUE = '\033[1;34m'


def set_timezone(timezone):
    assert sys.platform == 'win32', "set_timezone is Windows only function"
    subprocess.call(['cmd', '/S', '/C', 'tzutil', '/s', timezone])


def set_timezone_and_exit(timezone):
    assert sys.platform == 'win32', "set_timezone_and_exit is Windows only function"
    set_timezone(timezone)
    sys.exit(1)


def get_timezone():
    assert sys.platform == 'win32', "get_timezone is Windows only function"
    return subprocess.check_output(['cmd', '/S', '/C', 'tzutil', '/g'], universal_newlines=True)


def set_sighdl_to_reset_timezone(timezone):
    assert sys.platform == 'win32', "install_signal_handler_to_restore_timezone is Windows only function"
    signal.signal(signal.SIGINT, lambda signal, frame: set_timezone_and_exit(timezone))


# This is for not lost data on 'win32' with python 'print'.
# When use python subprocess call another script on win32, output with
# 'utf-8' encoding, that's the same like linux platform; but when
# call the python script in 'cmd.exe' shell, we have to output in 'native' encoding.
def setup_stdio():
    # For tty using native encoding, otherwise (pipe) use 'utf-8'
    encoding = sys.stdout.encoding if sys.stdout.isatty() else 'utf-8'
    # Always override it to avoid encode error
    sys.stdout = codecs.getwriter(encoding)(sys.stdout.buffer, 'xmlcharrefreplace')
    sys.stderr = codecs.getwriter(encoding)(sys.stderr.buffer, 'xmlcharrefreplace')


def print_test_summary(summary_string, total, passed, failed):
    print(f"\n[summary] {summary_string}\n")
    print(f"TOTAL: {total}")
    print(f"{TERM_GREEN}PASS: {passed}{TERM_NORMAL}")
    print(f"{TERM_RED}FAIL: {failed}{TERM_NORMAL}\n")

    success_color = TERM_GREEN if passed == total else TERM_RED
    print(f"{success_color}Success: {passed*100/total}{TERM_NORMAL}")


def print_test_result(tested, total, is_passed, passed_string, test_path, is_snapshot_generation=None):
    if is_snapshot_generation is None:
        snapshot_string = ''
    elif is_snapshot_generation:
        snapshot_string = ' (generate snapshot)'
    else:
        snapshot_string = ' (execute snapshot)'

    color = TERM_GREEN if is_passed else TERM_RED
    print(f"[{tested:>4}/{total:>4}] {color}{passed_string}: {test_path}{snapshot_string}{TERM_NORMAL}")


def get_platform_cmd_prefix():
    if sys.platform == 'win32':
        return ['cmd', '/S', '/C']
    return []


def get_python_cmd_prefix():
    # python script doesn't have execute permission on github actions windows runner
    return [sys.executable or 'python']
