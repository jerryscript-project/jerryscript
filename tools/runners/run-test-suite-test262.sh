#!/bin/bash

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

ENGINE="$1"
PATH_TO_TEST262="$2"
OUTPUT_DIR=`dirname $ENGINE`
REPORT_PATH="${OUTPUT_DIR}/test262.report"
TIMEOUT="90s"

if [ $# -lt 2 ]
then
  echo "This script performs parallel test262 compliance testing of the specified engine."
  echo ""
  echo "Usage:"
  echo "  1st parameter: JavaScript engine to be tested."
  echo "  2nd parameter: path to the directory with official test262 testsuite."
  echo ""
  echo "Example:"
  echo "  ./run-test-suite-test262.sh <engine> <test262_dir>"
  exit 1
fi

if [ ! -d "${PATH_TO_TEST262}/.git" ]
then
  git clone https://github.com/tc39/test262.git -b es5-tests "${PATH_TO_TEST262}"
fi

rm -rf "${PATH_TO_TEST262}/test/suite/bestPractice"
rm -rf "${PATH_TO_TEST262}/test/suite/intl402"

echo "Starting test262 testing for ${ENGINE}. Running test262 may take a several minutes."

"${PATH_TO_TEST262}"/tools/packaging/test262.py --command "timeout ${TIMEOUT} ${ENGINE}" \
                                                --tests="${PATH_TO_TEST262}" --summary \
                                                &> "${REPORT_PATH}"

grep -A3 "=== Summary ===" "${REPORT_PATH}"

FAILURES=`sed -n '/Failed tests/,/^$/p' "${REPORT_PATH}"`

EXIT_CODE=0
if [ -n "$FAILURES" ]
then
  echo -e "\n$FAILURES\n"
  echo "$0: see ${REPORT_PATH} for details about failures"
  EXIT_CODE=1
fi

FAILURES=`sed -n '/Expected to fail but passed/,/^$/p' "${REPORT_PATH}"`
if [ -n "$FAILURES" ]
then
  echo -e "\n$FAILURES\n"
  echo "$0: see ${REPORT_PATH} for details about failures"
  EXIT_CODE=1
fi

exit $EXIT_CODE
