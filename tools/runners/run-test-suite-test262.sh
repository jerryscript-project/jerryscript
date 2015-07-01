#!/bin/bash

# Copyright 2015 Samsung Electronics Co., Ltd.
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
REPORT_PREFIX="report"
RUN_PIDS=""
RESULT_OK=1
TIMEOUT="5s"

if [ $# -lt 2 ]
then
  echo "This script performs parallel test262 compliance testing of the specified engine."
  echo ""
  echo "Usage:"
  echo "  1st parameter: JavaScript engine to be tested."
  echo "  2nd parameter: path to the directory with official test262 testsuite."
  echo "  3rd parameter: (optional) call this script with the '--sub-chapters' flag to print the detailed report."
  echo ""
  echo "Example:"
  echo "  ./run-test-suite-test262.sh <engine> <test262_dir> --sub-chapters"
  exit 1
fi


rm "${REPORT_PREFIX}".* &> /dev/null

declare -a CHAPTER07_TO_TEST=("7.1" "7.2" "7.3" "7.4" "7.5" "7.6" "7.6.1" "7.7"                                        \
                              "7.8" "7.8.1" "7.8.2" "7.8.3" "7.8.4" "7.8.5" "7.9" "7.9.2")
declare -a CHAPTER08_TO_TEST=("8.1" "8.2" "8.3" "8.4" "8.5" "8.6" "8.6.1" "8.6.2" "8.7" "8.7.1" "8.7.2" "8.8" "8.12"   \
                              "8.12.1" "8.12.3" "8.12.4" "8.12.5" "8.12.6" "8.12.7" "8.12.8" "8.12.9")
declare -a CHAPTER09_TO_TEST=("9.1" "9.2" "9.3" "9.3.1" "9.4" "9.5" "9.6" "9.7" "9.8" "9.8.1" "9.9")
declare -a CHAPTER10_TO_TEST=("10.1" "10.1.1" "10.2" "10.2.1" "10.2.2" "10.2.3" "10.3" "10.3.1" "10.4" "10.4.1"        \
                              "10.4.2" "10.4.3" "10.5" "10.6")
declare -a CHAPTER11_TO_TEST=("11.1" "11.1.1" "11.1.2" "11.1.3" "11.1.4" "11.1.5" "11.1.6" "11.2" "11.2.1" "11.2.2"    \
                              "11.2.3" "11.2.4" "11.3" "11.3.1" "11.3.2" "11.4" "11.4.1" "11.4.2" "11.4.3"             \
                              "11.4.4" "11.4.5" "11.4.6" "11.4.7" "11.4.8" "11.4.9" "11.5" "11.5.1" "11.5.2" "11.5.3"  \
                              "11.6" "11.6.1" "11.6.2" "11.7" "11.7.1" "11.7.2" "11.7.3" "11.8" "11.8.1" "11.8.2"      \
                              "11.8.3" "11.8.4" "11.8.6" "11.8.7" "11.9" "11.9.1" "11.9.2" "11.9.4" "11.9.5" "11.10"   \
                              "11.11" "11.12" "11.13" "11.13.1" "11.13.2" "11.14")
declare -a CHAPTER12_TO_TEST=("12.1" "12.2" "12.2.1" "12.3" "12.4" "12.5" "12.6" "12.6.1" "12.6.2" "12.6.3" "12.6.4"   \
                              "12.7" "12.8" "12.9" "12.10" "12.10.1" "12.11" "12.12" "12.13" "12.14" "12.14.1")
declare -a CHAPTER13_TO_TEST=("13.1" "13.2" "13.2.1" "13.2.2" "13.2.3")
declare -a CHAPTER14_TO_TEST=("14.1")
declare -a CHAPTER14_TO_TEST=("12.6.4")
declare -a CHAPTER15_TO_TEST=("15.1" "15.1.1" "15.1.2" "15.1.3" "15.1.4" "15.1.5" "15.2" "15.2.1" "15.2.2" "15.2.3"    \
                              "15.2.4" "15.2.5" "15.3" "15.3.1" "15.3.2" "15.3.3" "15.3.4" "15.3.5" "15.4" "15.4.1"    \
                              "15.4.2" "15.4.3" "15.4.4" "15.4.5" "15.5" "15.5.1" "15.5.2" "15.5.3" "15.5.4" "15.5.5"  \
                              "15.6" "15.6.1" "15.6.2" "15.6.3" "15.6.4" "15.6.5" "15.7" "15.7.1" "15.7.2" "15.7.3"    \
                              "15.7.4" "15.7.5" "15.8" "15.8.1" "15.8.2" "15.9" "15.9.1" "15.9.2" "15.9.3" "15.9.4"    \
                              "15.9.5" "15.9.6" "15.10" "15.10.1" "15.10.2" "15.10.3" "15.10.4" "15.10.5" "15.10.6"    \
                              "15.10.7" "15.11" "15.11.1" "15.11.2" "15.11.3" "15.11.4" "15.11.5" "15.11.6" "15.11.7"  \
                              "15.12" "15.12.1" "15.12.2" "15.12.3")
declare -a FULL_CHAPTERS_TO_TEST=("ch06" "ch07" "ch08" "ch09" "ch10" "ch11" "ch12" "ch13" "ch14" "ch15")
declare -a SUB_CHAPTERS_TO_TEST=("${CHAPTER07_TO_TEST[@]}" "${CHAPTER08_TO_TEST[@]}" "${CHAPTER09_TO_TEST[@]}"         \
                                 "${CHAPTER10_TO_TEST[@]}" "${CHAPTER11_TO_TEST[@]}" "${CHAPTER12_TO_TEST[@]}"         \
                                 "${CHAPTER13_TO_TEST[@]}" "${CHAPTER14_TO_TEST[@]}" "${CHAPTER15_TO_TEST[@]}")

declare -a CHAPTERS_TO_TEST=("${FULL_CHAPTERS_TO_TEST[@]}")

if [[ $* == *--sub-chapters* ]]
then
  declare -a CHAPTERS_TO_TEST=("${SUB_CHAPTERS_TO_TEST[@]}")
fi

function run_test262 () {
  ARG_ENGINE="$1"
  ARG_TEST262_PATH="$2"
  ARG_CHAPTER="$3"

  "${ARG_TEST262_PATH}"/tools/packaging/test262.py --command "timeout ${TIMEOUT} ${ARG_ENGINE}"                        \
                                                   --tests="${ARG_TEST262_PATH}" --full-summary "${ARG_CHAPTER}"       \
                                                   > "${REPORT_PREFIX}"."${ARG_CHAPTER}"
}

function show_report_results () {
  ARG_CHAPTER="$1"

  echo ""
  echo "Chapter ${ARG_CHAPTER}:"
  grep -A3 "=== Summary ===" "${REPORT_PREFIX}"."${ARG_CHAPTER}"
  echo "==============="
}

echo "Starting test262 testing for ${ENGINE}."

for TEST_NAME in "${CHAPTERS_TO_TEST[@]}"
do
  run_test262 "${ENGINE}" "${PATH_TO_TEST262}" "$TEST_NAME" &
  RUN_PIDS="$RUN_PIDS $!";
done

for RUN_PIDS in $RUN_PIDS
do
  wait "$RUN_PIDS" || RESULT_OK=0
done;
#[ $RESULT_OK -eq 1 ] || exit 1

echo "Testing is completed."

for TEST_NAME in "${CHAPTERS_TO_TEST[@]}"
do
  echo "$TEST_NAME"
  show_report_results "$TEST_NAME"
done
