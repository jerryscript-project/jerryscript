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

# Usage:
#       ./tools/runners/run-test-suite.sh ENGINE TESTS [--snapshot] ENGINE_ARGS....

TIMEOUT=${TIMEOUT:=5}

ENGINE="$1"
shift

TESTS="$1"
shift

OUTPUT_DIR=`dirname $ENGINE`
TESTS_BASENAME=`basename $TESTS`

TEST_FILES=$OUTPUT_DIR/$TESTS_BASENAME.files
TEST_FAILED=$OUTPUT_DIR/$TESTS_BASENAME.failed
TEST_PASSED=$OUTPUT_DIR/$TESTS_BASENAME.passed

if [ "$1" == "--snapshot" ]
then
    TEST_FILES="$TEST_FILES.snapshot"
    TEST_FAILED="$TEST_FAILED.snapshot"
    TEST_PASSED="$TEST_PASSED.snapshot"
    IS_SNAPSHOT=true;
    shift
fi

ENGINE_ARGS="$@"

if [ ! -x $ENGINE ]
then
    echo "$0: $ENGINE: not an executable"
    exit 1
fi

if [ -d $TESTS ]
then
    TESTS_DIR=$TESTS

    ( cd $TESTS; find . -name "[^N]*.js" ) | sort > $TEST_FILES
elif [ -f $TESTS ]
then
    TESTS_DIR=`dirname $TESTS`

    grep -e '.js\s*$' $TESTS | sort > $TEST_FILES
else
    echo "$0: $TESTS: not a test suite"
    exit 1
fi

total=$(cat $TEST_FILES | wc -l)
if [ "$total" -eq 0 ]
then
    echo "$0: $TESTS: no test in test suite"
    exit 1
fi

rm -f $TEST_FAILED $TEST_PASSED

ROOT_DIR=""
CURRENT_DIR=`pwd`
PATH_STEP=2
while true
do
    TMP_ROOT_DIR=`(echo "$CURRENT_DIR"; echo "$0"; echo "$ENGINE"; echo "$TESTS") | cut -f1-$PATH_STEP -d/ | uniq -d`
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

ENGINE_TEMP=`mktemp engine-out.XXXXXXXXXX`

for test in `cat $TEST_FILES`
do
    error_code=`echo $test | grep -e "^.\/fail\/[0-9]*\/" -o | cut -d / -f 3`
    if [ "$error_code" = "" ]
    then
        PASS="PASS"
        error_code=0
    else
        PASS="PASS (XFAIL)"
    fi

    full_test=$TESTS_DIR/${test#./}

    if [ "$IS_SNAPSHOT" == true ]
    then
        # Testing snapshot
        SNAPSHOT_TEMP=`mktemp $(basename -s .js $test).snapshot.XXXXXXXXXX`

        cmd_line="${ENGINE#$ROOT_DIR} $ENGINE_ARGS --save-snapshot-for-global $SNAPSHOT_TEMP ${full_test#$ROOT_DIR}"
        ( ulimit -t $TIMEOUT; $ENGINE $ENGINE_ARGS --save-snapshot-for-global $SNAPSHOT_TEMP $full_test &> $ENGINE_TEMP )
        status_code=$?

        if [ $status_code -eq 0 ]
        then
            echo "[$tested/$total] $cmd_line: PASS"

            cmd_line="${ENGINE#$ROOT_DIR} $ENGINE_ARGS --exec-snapshot $SNAPSHOT_TEMP"
            ( ulimit -t $TIMEOUT; $ENGINE $ENGINE_ARGS --exec-snapshot $SNAPSHOT_TEMP &> $ENGINE_TEMP )
            status_code=$?
        fi

        rm -f $SNAPSHOT_TEMP
    else
        cmd_line="${ENGINE#$ROOT_DIR} $ENGINE_ARGS ${full_test#$ROOT_DIR}"
        ( ulimit -t $TIMEOUT; $ENGINE $ENGINE_ARGS $full_test &> $ENGINE_TEMP )
        status_code=$?
    fi

    if [ $status_code -ne $error_code ]
    then
        echo "[$tested/$total] $cmd_line: FAIL ($status_code)"
        cat $ENGINE_TEMP

        echo "$status_code: $test" >> $TEST_FAILED
        echo "============================================" >> $TEST_FAILED
        cat $ENGINE_TEMP >> $TEST_FAILED
        echo "============================================" >> $TEST_FAILED
        echo >> $TEST_FAILED
        echo >> $TEST_FAILED

        failed=$((failed+1))
    else
        echo "[$tested/$total] $cmd_line: $PASS"

        echo "$test" >> $TEST_PASSED

        passed=$((passed+1))
    fi

    tested=$((tested+1))
done

rm -f $ENGINE_TEMP

ratio=$(echo $passed*100/$total | bc)

if [ "$IS_SNAPSHOT" == true ]
then
    ENGINE_ARGS="--snapshot $ENGINE_ARGS"
fi

echo "[summary] ${ENGINE#$ROOT_DIR} $ENGINE_ARGS ${TESTS#$ROOT_DIR}: $passed PASS, $failed FAIL, $total total, $ratio% success"

if [ $failed -ne 0 ]
then
    echo "$0: see $TEST_FAILED for details about failures"
    exit 1
fi

exit 0
