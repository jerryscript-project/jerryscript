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

DIR="$1"
shift

UNITTEST_ERROR=$DIR/unittests.failed
UNITTEST_OK=$DIR/unittests.passed

rm -f $UNITTEST_ERROR $UNITTEST_OK

UNITTESTS=$(ls $DIR/unit-*)
total=$(ls $DIR/unit-* | wc -l)

if [ "$total" -eq 0 ]
then
    echo "$0: $DIR: no unit-* test to execute"
    exit 1
fi

ROOT_DIR=""
CURRENT_DIR=`pwd`
PATH_STEP=2
while true
do
    TMP_ROOT_DIR=`(echo "$CURRENT_DIR"; echo "$0"; echo "$DIR") | cut -f1-$PATH_STEP -d/ | uniq -d`
    if [ -z "$TMP_ROOT_DIR" ]
    then
        break
    else
        ROOT_DIR="$TMP_ROOT_DIR"
    fi
    PATH_STEP=$((PATH_STEP+1))
done
if [ -n "$ROOT_DIR" ]
then
    ROOT_DIR="$ROOT_DIR/"
fi

tested=1
failed=0
passed=0

UNITTEST_TEMP=`mktemp unittest-out.XXXXXXXXXX`

for unit_test in $UNITTESTS
do
    cmd_line="${unit_test#$ROOT_DIR}"
    $unit_test &>$UNITTEST_TEMP
    status_code=$?

    if [ $status_code -ne 0 ]
    then
        echo "[$tested/$total] $cmd_line: FAIL ($status_code)"
        cat $UNITTEST_TEMP

        echo "$status_code: $unit_test" >> $UNITTEST_ERROR
        echo "============================================" >> $UNITTEST_ERROR
        cat $UNITTEST_TEMP >> $UNITTEST_ERROR
        echo "============================================" >> $UNITTEST_ERROR
        echo >> $UNITTEST_ERROR
        echo >> $UNITTEST_ERROR

        failed=$((failed+1))
    else
        echo "[$tested/$total] $cmd_line: PASS"

        echo "$unit_test" >> $UNITTEST_OK

        passed=$((passed+1))
    fi

    tested=$((tested+1))
done

rm -f $UNITTEST_TEMP

ratio=$(echo $passed*100/$total | bc)

echo "[summary] ${DIR#$ROOT_DIR}/unit-*: $passed PASS, $failed FAIL, $total total, $ratio% success"

if [ $failed -ne 0 ]
then
    echo "$0: see $UNITTEST_ERROR for details about failures"
    exit 1
fi

exit 0
