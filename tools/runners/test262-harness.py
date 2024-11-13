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


# This file is based on work under the following copyright and permission notice:
# https://github.com/test262-utils/test262-harness-py/blob/master/src/test262.py

# license of test262.py:
# Copyright 2009 the Sputnik authors.  All rights reserved.
# This code is governed by the BSD license found in the LICENSE file.
# This is derived from sputnik.py, the Sputnik console test runner,
# with elements from packager.py, which is separately
# copyrighted. TODO: Refactor so there is less duplication between
# test262.py and packager.py.

# license of _packager.py:
# Copyright (c) 2012 Ecma International.  All rights reserved.
# This code is governed by the BSD license found in the LICENSE file.


import logging
import argparse
import os
from os import path
import platform
import re
import subprocess
import sys
import tempfile
import xml.dom.minidom
from collections import Counter

import signal
import multiprocessing

import util

# The timeout of each test case
TEST262_CASE_TIMEOUT = 180

TEST_RE = re.compile(r"(?P<test1>.*)\/\*---(?P<header>.+)---\*\/(?P<test2>.*)", re.DOTALL)
YAML_INCLUDES_RE = re.compile(r"includes:\s+\[(?P<includes>.+)\]")
YAML_FLAGS_RE = re.compile(r"flags:\s+\[(?P<flags>.+)\]")
YAML_NEGATIVE_RE = re.compile(r"negative:.*phase:\s+(?P<phase>\w+).*type:\s+(?P<type>\w+)", re.DOTALL)

class Test262Error(Exception):
    def __init__(self, message):
        Exception.__init__(self)
        self.message = message


def report_error(error_string):
    raise Test262Error(error_string)


def build_options():
    result = argparse.ArgumentParser()
    result.add_argument("--command", default=None,
                        help="The command-line to run")
    result.add_argument("--tests", default=path.abspath('.'),
                        help="Path to the tests")
    result.add_argument("--exclude-list", default=None,
                        help="Path to the excludelist.xml file")
    result.add_argument("--cat", default=False, action="store_true",
                        help="Print packaged test code that would be run")
    result.add_argument("--summary", default=False, action="store_true",
                        help="Print summary after running tests")
    result.add_argument("--full-summary", default=False, action="store_true",
                        help="Print summary and test output after running tests")
    result.add_argument("--strict_only", default=False, action="store_true",
                        help="Test only strict mode")
    result.add_argument("--non_strict_only", default=False, action="store_true",
                        help="Test only non-strict mode")
    result.add_argument("--unmarked_default", default="both",
                        help="default mode for tests of unspecified strictness")
    result.add_argument("-j", "--job-count", default=None, action="store", type=int,
                        help="Number of parallel test jobs to run. In case of '0' cpu count is used.")
    result.add_argument("--logname", help="Filename to save stdout to")
    result.add_argument("--loglevel", default="warning",
                        help="sets log level to debug, info, warning, error, or critical")
    result.add_argument("--print-handle", default="print",
                        help="Command to print from console")
    result.add_argument("--list-includes", default=False, action="store_true",
                        help="List includes required by tests")
    result.add_argument("--module-flag", default="-m",
                        help="List includes required by tests")
    result.add_argument("test_list", nargs='*', default=None)
    return result


def validate_options(options):
    if not options.command:
        report_error("A --command must be specified.")
    if not path.exists(options.tests):
        report_error(f"Couldn't find test path '{options.tests}'")


def is_windows():
    actual_platform = platform.system()
    return actual_platform in ('Windows', 'Microsoft')


class TempFile:

    def __init__(self, suffix="", prefix="tmp", text=False):
        self.suffix = suffix
        self.prefix = prefix
        self.text = text
        self.file_desc = None
        self.name = None
        self.is_closed = False
        self.open_file()

    def open_file(self):
        (self.file_desc, self.name) = tempfile.mkstemp(
            suffix=self.suffix,
            prefix=self.prefix,
            text=self.text)

    def write(self, string):
        os.write(self.file_desc, string.encode('utf8', 'ignore'))

    def read(self):
        with open(self.name, "r", newline='', encoding='utf8', errors='ignore') as file_desc:
            return file_desc.read()

    def close(self):
        if not self.is_closed:
            self.is_closed = True
            os.close(self.file_desc)

    def dispose(self):
        try:
            self.close()
            os.unlink(self.name)
        except OSError as os_error:
            logging.error("Error disposing temp file: %s", os_error)


class TestResult:

    def __init__(self, exit_code, stdout, stderr, case):
        self.exit_code = exit_code
        self.stdout = stdout
        self.stderr = stderr
        self.case = case

    def report_outcome(self, long_format):
        name = self.case.get_name()
        mode = self.case.get_mode()

        if self.exit_code not in (0, 1):
            sys.stderr.write(f"==={name} failed in {mode} with negative:{self.case.get_negative_type()}===\n")
            self.write_output(sys.stderr)

        if self.has_unexpected_outcome():
            if self.case.is_negative():
                print(f"=== {name} passed in {mode}, but was expected to fail ===")
                print(f"--- expected error: {self.case.get_negative_type()} ---\n")
            else:
                if long_format:
                    print(f"=== {name} failed in {mode} ===")
                else:
                    print(f"{name} in {mode}: ")
            self.write_output(sys.stdout)
            if long_format:
                print("===")
        elif self.case.is_negative():
            print(f"{name} failed in {mode} as expected")
        else:
            print(f"{name} passed in {mode}")

    def write_output(self, target):
        out = self.stdout.strip()
        if out:
            target.write(f"--- output --- \n {out}")
        error = self.stderr.strip()
        if error:
            target.write(f"--- errors ---  \n {error}")

        target.write(f"\n--- exit code: {self.exit_code} ---\n")

    def has_failed(self):
        return self.exit_code != 0

    def async_has_failed(self):
        return 'Test262:AsyncTestComplete' not in self.stdout

    def has_unexpected_outcome(self):
        if self.case.is_negative():
            return not (self.has_failed() and self.case.negative_match(self.get_error_output()))

        if self.case.is_async_test():
            return self.async_has_failed() or self.has_failed()

        return self.has_failed()

    def get_error_output(self):
        if self.stderr:
            return self.stderr
        return self.stdout


class TestCase:

    def __init__(self, suite, name, full_path, strict_mode, command_template, module_flag):
        self.suite = suite
        self.name = name
        self.full_path = full_path
        self.strict_mode = strict_mode
        self.command_template = command_template
        self.module_flag = module_flag
        self.test_record = {}
        self.parse_test_record()
        self.validate()

    def parse_test_record(self):
        with open(self.full_path, "r", newline='', encoding='utf8', errors='ignore') as file_desc:
            full_test = file_desc.read()

        match = TEST_RE.search(full_test)
        header = match.group("header")
        self.test = match.group("test1") + match.group("test2")

        match = YAML_INCLUDES_RE.search(header)

        if match:
            self.test_record["includes"] = [inc.strip() for inc in match.group("includes").split(",") if inc]

        match = YAML_FLAGS_RE.search(header)
        self.test_record["flags"] = [flag.strip() for flag in match.group("flags").split(",") if flag] if match else []

        match = YAML_NEGATIVE_RE.search(header)
        if match:
            self.test_record["negative"] = {
                "phase" : match.group("phase"),
                "type" : match.group("type")
            }

    def negative_match(self, stderr):
        neg = re.compile(self.get_negative_type())
        return re.search(neg, stderr)

    def get_negative(self):
        if not self.is_negative():
            return None
        return self.test_record["negative"]

    def get_negative_type(self):
        negative = self.get_negative()
        if isinstance(negative, dict) and "type" in negative:
            return negative["type"]
        return negative

    def get_negative_phase(self):
        negative = self.get_negative()
        return negative and "phase" in negative and negative["phase"]

    def get_name(self):
        return path.join(*self.name)

    def get_mode(self):
        if self.strict_mode:
            return "strict mode"
        return "non-strict mode"

    def get_path(self):
        return self.name

    def is_negative(self):
        return 'negative' in self.test_record

    def is_only_strict(self):
        return 'onlyStrict' in self.test_record["flags"]

    def is_no_strict(self):
        return 'noStrict' in self.test_record["flags"] or self.is_raw()

    def is_raw(self):
        return 'raw' in self.test_record["flags"]

    def is_async_test(self):
        return 'async' in self.test_record["flags"] or '$DONE' in self.test

    def is_module(self):
        return 'module' in self.test_record["flags"]

    def get_include_list(self):
        if self.test_record.get('includes'):
            return self.test_record['includes']
        return []

    def get_additional_includes(self):
        return '\n'.join([self.suite.get_include(include) for include in self.get_include_list()])

    def get_source(self):
        if self.is_raw():
            return self.test

        source = self.suite.get_include("sta.js") + \
            self.suite.get_include("assert.js")

        if self.is_async_test():
            source = source + \
                self.suite.get_include("timer.js") + \
                self.suite.get_include("doneprintHandle.js").replace(
                    'print', self.suite.print_handle)

        source = source + \
            self.get_additional_includes() + \
            self.test + '\n'

        if self.get_negative_phase() == "early":
            source = ("throw 'Expected an early error, but code was executed.';\n" +
                      source)

        if self.strict_mode:
            source = '"use strict";\nvar strict_mode = true;\n' + source
        else:
            # add comment line so line numbers match in both strict and non-strict version
            source = '//"no strict";\nvar strict_mode = false;\n' + source

        return source

    @staticmethod
    def instantiate_template(template, params):
        def get_parameter(match):
            key = match.group(1)
            return params.get(key, match.group(0))

        return re.sub(r"\{\{(\w+)\}\}", get_parameter, template)

    @staticmethod
    def execute(command):
        if is_windows():
            args = f'{command}'
        else:
            args = command.split(" ")
        stdout = TempFile(prefix="test262-out-")
        stderr = TempFile(prefix="test262-err-")
        try:
            logging.info("exec: %s", str(args))
            with subprocess.Popen(
                args,
                shell=False,
                stdout=stdout.file_desc,
                stderr=stderr.file_desc
            ) as process:
                try:
                    code = process.wait(timeout=TEST262_CASE_TIMEOUT)
                except subprocess.TimeoutExpired:
                    process.kill()
                out = stdout.read()
                err = stderr.read()
        finally:
            stdout.dispose()
            stderr.dispose()
        return (code, out, err)

    def run_test_in(self, tmp):
        tmp.write(self.get_source())
        tmp.close()

        if self.is_module():
            arg = self.module_flag + ' ' + tmp.name
        else:
            arg = tmp.name

        command = TestCase.instantiate_template(self.command_template, {
            'path': arg
        })

        (code, out, err) = TestCase.execute(command)
        return TestResult(code, out, err, self)

    def run(self):
        tmp = TempFile(suffix=".js", prefix="test262-")
        try:
            result = self.run_test_in(tmp)
        finally:
            tmp.dispose()
        return result

    def print_source(self):
        print(self.get_source())

    def validate(self):
        flags = self.test_record.get("flags")
        phase = self.get_negative_phase()

        if phase not in [None, False, "parse", "early", "runtime", "resolution"]:
            raise TypeError("Invalid value for negative phase: " + phase)

        if not flags:
            return

        if 'raw' in flags:
            if 'noStrict' in flags:
                raise TypeError("The `raw` flag implies the `noStrict` flag")
            if 'onlyStrict' in flags:
                raise TypeError(
                    "The `raw` flag is incompatible with the `onlyStrict` flag")
            if self.get_include_list():
                raise TypeError(
                    "The `raw` flag is incompatible with the `includes` tag")


def pool_init():
    """Ignore CTRL+C in the worker process."""
    signal.signal(signal.SIGINT, signal.SIG_IGN)


def test_case_run_process(case):
    return case.run()


class ProgressIndicator:

    def __init__(self, count):
        self.count = count
        self.succeeded = 0
        self.failed = 0
        self.failed_tests = []

    def has_run(self, result):
        result.report_outcome(True)
        if result.has_unexpected_outcome():
            self.failed += 1
            self.failed_tests.append(result)
        else:
            self.succeeded += 1


def percent_format(partial, total):
    return f"{partial} test{'s' if partial > 1 else ''} ({(100.0 * partial)/total:.1f}%)"


class TestSuite:

    def __init__(self, options):
        self.test_root = path.join(options.tests, 'test')
        self.lib_root = path.join(options.tests, 'harness')
        self.strict_only = options.strict_only
        self.non_strict_only = options.non_strict_only
        self.unmarked_default = options.unmarked_default
        self.print_handle = options.print_handle
        self.include_cache = {}
        self.exclude_list_path = options.exclude_list
        self.module_flag = options.module_flag
        self.logf = None

    def _load_excludes(self):
        if self.exclude_list_path and os.path.exists(self.exclude_list_path):
            xml_document = xml.dom.minidom.parse(self.exclude_list_path)
            xml_tests = xml_document.getElementsByTagName("test")
            return {x.getAttribute("id") for x in xml_tests}

        return set()

    def validate(self):
        if not path.exists(self.test_root):
            report_error("No test repository found")
        if not path.exists(self.lib_root):
            report_error("No test library found")

    @staticmethod
    def is_hidden(test_path):
        return test_path.startswith('.') or test_path == 'CVS'

    @staticmethod
    def is_test_case(test_path):
        return test_path.endswith('.js') and not test_path.endswith('_FIXTURE.js')

    @staticmethod
    def should_run(rel_path, tests):
        if not tests:
            return True
        for test in tests:
            if os.path.normpath(test) in os.path.normpath(rel_path):
                return True
        return False

    def get_include(self, name):
        if not name in self.include_cache:
            static = path.join(self.lib_root, name)
            if path.exists(static):
                with open(static, encoding='utf8', errors='ignore') as file_desc:
                    contents = file_desc.read()
                    contents = re.sub(r'\r\n', '\n', contents)
                    self.include_cache[name] = contents + "\n"
            else:
                report_error("Can't find: " + static)
        return self.include_cache[name]

    def enumerate_tests(self, tests, command_template):
        exclude_list = self._load_excludes()

        logging.info("Listing tests in %s", self.test_root)
        cases = []
        for root, dirs, files in os.walk(self.test_root):
            for hidden_dir in [x for x in dirs if self.is_hidden(x)]:
                dirs.remove(hidden_dir)
            dirs.sort()
            for test_path in filter(TestSuite.is_test_case, sorted(files)):
                full_path = path.join(root, test_path)
                if full_path.startswith(self.test_root):
                    rel_path = full_path[len(self.test_root)+1:]
                else:
                    logging.warning("Unexpected path %s", full_path)
                    rel_path = full_path
                if self.should_run(rel_path, tests):
                    basename = path.basename(full_path)[:-3]
                    name = rel_path.split(path.sep)[:-1] + [basename]
                    if rel_path in exclude_list:
                        print('Excluded: ' + rel_path)
                    else:
                        if not self.non_strict_only:
                            strict_case = TestCase(self, name, full_path, True, command_template, self.module_flag)
                            if not strict_case.is_no_strict():
                                if strict_case.is_only_strict() or self.unmarked_default in ['both', 'strict']:
                                    cases.append(strict_case)
                        if not self.strict_only:
                            non_strict_case = TestCase(self, name, full_path, False, command_template, self.module_flag)
                            if not non_strict_case.is_only_strict():
                                if non_strict_case.is_no_strict() or self.unmarked_default in ['both', 'non_strict']:
                                    cases.append(non_strict_case)
        logging.info("Done listing tests")
        return cases

    def print_summary(self, progress, logfile):

        def write(string):
            if logfile:
                self.logf.write(string + "\n")
            print(string)

        print("")
        write("=== Summary ===")
        count = progress.count
        succeeded = progress.succeeded
        failed = progress.failed
        write(f" - Ran {count} test{'s' if count > 1 else ''}")
        if progress.failed == 0:
            write(" - All tests succeeded")
        else:
            write(" - Passed " + percent_format(succeeded, count))
            write(" - Failed " + percent_format(failed, count))
            positive = [c for c in progress.failed_tests if not c.case.is_negative()]
            negative = [c for c in progress.failed_tests if c.case.is_negative()]
            if positive:
                print("")
                write("Failed Tests")
                for result in positive:
                    write(f"  {result.case.get_name()} in {result.case.get_mode()}")
            if negative:
                print("")
                write("Expected to fail but passed ---")
                for result in negative:
                    write(f"  {result.case.get_name()} in {result.case.get_mode()}")

    def print_failure_output(self, progress, logfile):
        for result in progress.failed_tests:
            if logfile:
                self.write_log(result)
            print("")
            result.report_outcome(False)

    def run(self, command_template, tests, print_summary, full_summary, logname, job_count=1):
        if not "{{path}}" in command_template:
            command_template += " {{path}}"
        cases = self.enumerate_tests(tests, command_template)
        if not cases:
            report_error("No tests to run")
        progress = ProgressIndicator(len(cases))
        if logname:
            self.logf = open(logname, "w", encoding='utf8', errors='ignore')  # pylint: disable=consider-using-with

        if job_count == 1:
            for case in cases:
                result = case.run()
                if logname:
                    self.write_log(result)
                progress.has_run(result)
        else:
            if job_count == 0:
                job_count = None # uses multiprocessing.cpu_count()

            with multiprocessing.Pool(processes=job_count, initializer=pool_init) as pool:
                for result in pool.imap(test_case_run_process, cases):
                    if logname:
                        self.write_log(result)
                    progress.has_run(result)

        if print_summary:
            self.print_summary(progress, logname)
            if full_summary:
                self.print_failure_output(progress, logname)
            else:
                print("")
                print("Use --full-summary to see output from failed tests")
        print("")
        return progress.failed

    def write_log(self, result):
        name = result.case.get_name()
        mode = result.case.get_mode()
        if result.has_unexpected_outcome():
            if result.case.is_negative():
                self.logf.write(
                    f"=== {name} passed in {mode}, but was expected to fail === \n")
                self.logf.write(f"--- expected error: {result.case.GetNegativeType()} ---\n")
                result.write_output(self.logf)
            else:
                self.logf.write(f"=== {name} failed in {mode} === \n")
                result.write_output(self.logf)
            self.logf.write("===\n")
        elif result.case.is_negative():
            self.logf.write(f"{name} failed in {mode} as expected \n")
        else:
            self.logf.write(f"{name} passed in {mode} \n")

    def print_source(self, tests):
        cases = self.enumerate_tests(tests, "")
        if cases:
            cases[0].print_source()

    def list_includes(self, tests):
        cases = self.enumerate_tests(tests, "")
        includes_dict = Counter()
        for case in cases:
            includes = case.get_include_list()
            includes_dict.update(includes)

        print(includes_dict)


def main():
    util.setup_stdio()
    code = 0
    parser = build_options()
    options = parser.parse_args()
    args = options.test_list
    validate_options(options)

    test_suite = TestSuite(options)

    test_suite.validate()
    if options.loglevel == 'debug':
        logging.basicConfig(level=logging.DEBUG)
    elif options.loglevel == 'info':
        logging.basicConfig(level=logging.INFO)
    elif options.loglevel == 'warning':
        logging.basicConfig(level=logging.WARNING)
    elif options.loglevel == 'error':
        logging.basicConfig(level=logging.ERROR)
    elif options.loglevel == 'critical':
        logging.basicConfig(level=logging.CRITICAL)

    if options.cat:
        test_suite.print_source(args)
    elif options.list_includes:
        test_suite.list_includes(args)
    else:
        code = test_suite.run(options.command, args,
                              options.summary or options.full_summary,
                              options.full_summary,
                              options.logname,
                              options.job_count)
    return code


if __name__ == '__main__':
    try:
        sys.exit(main())
    except Test262Error as exception:
        print(f"Error: {exception.message}")
        sys.exit(1)
