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
#       ./tools/runners/run-test-suite.sh ENGINE TESTS [--skip-list=item1,item2] [--snapshot] ENGINE_ARGS....

TIMEOUT=${TIMEOUT:=5}
TIMEOUT_CMD=`which timeout`
if [ $? -ne 0 ]
then
    TIMEOUT_CMD=`which gtimeout`
fi

ENGINE="$1"
shift

TESTS="$1"
shift

OUTPUT_DIR=`dirname $ENGINE`
TESTS_BASENAME=`basename $TESTS`

TEST_FILES=$OUTPUT_DIR/$TESTS_BASENAME.files
TEST_FAILED=$OUTPUT_DIR/$TESTS_BASENAME.failed
TEST_PASSED=$OUTPUT_DIR/$TESTS_BASENAME.passed

if [[ "$1" =~ ^--skip-list=.* ]]
then
    SKIP_LIST=${1#--skip-list=}
    SKIP_LIST=(${SKIP_LIST//,/ })
    shift
fi

if [ "$1" == "--snapshot" ]
then
    TEST_FILES="$TEST_FILES.snapshot"
    TEST_FAILED="$TEST_FAILED.snapshot"
    TEST_PASSED="$TEST_PASSED.snapshot"
    IS_SNAPSHOT=true

    SNAPSHOT_TOOL=${ENGINE}-snapshot
    if [ ! -x $SNAPSHOT_TOOL ]
    then
        echo "$0: $SNAPSHOT_TOOL: not an executable"
        exit 1
    fi
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

# Remove the skipped tests from list
for TEST in "${SKIP_LIST[@]}"
do
    ( sed -i -r "/$TEST/d" $TEST_FILES )
done

TOTAL=$(cat $TEST_FILES | wc -l)
if [ "$TOTAL" -eq 0 ]
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
    if [[ $test =~ ^\.\/fail\/ ]]
    then
        error_code=1
        PASS="PASS (XFAIL)"
    else
        error_code=0
        PASS="PASS"
    fi

    full_test=$TESTS_DIR/${test#./}

    if [ "$IS_SNAPSHOT" == true ]
    then
        # Testing snapshot
        SNAPSHOT_TEMP=`mktemp $(basename -s .js $test).snapshot.XXXXXXXXXX`

        cmd_line="${SNAPSHOT_TOOL#$ROOT_DIR} generate --context global -o $SNAPSHOT_TEMP ${full_test#$ROOT_DIR}"
        $TIMEOUT_CMD $TIMEOUT $SNAPSHOT_TOOL generate --context global -o $SNAPSHOT_TEMP $full_test &> $ENGINE_TEMP
        status_code=$?

        if [ $status_code -eq 0 ]
        then
            echo "[$tested/$TOTAL] $cmd_line: PASS"

            cmd_line="${ENGINE#$ROOT_DIR} $ENGINE_ARGS --exec-snapshot $SNAPSHOT_TEMP"
            $TIMEOUT_CMD $TIMEOUT $ENGINE $ENGINE_ARGS --exec-snapshot $SNAPSHOT_TEMP &> $ENGINE_TEMP
            status_code=$?
        fi

        rm -f $SNAPSHOT_TEMP
    else
        cmd_line="${ENGINE#$ROOT_DIR} $ENGINE_ARGS ${full_test#$ROOT_DIR}"
        $TIMEOUT_CMD $TIMEOUT $ENGINE $ENGINE_ARGS $full_test &> $ENGINE_TEMP
        status_code=$?
    fi

    if [ $status_code -ne $error_code ]
    then
        echo "[$tested/$TOTAL] $cmd_line: FAIL ($status_code)"
        cat $ENGINE_TEMP

        echo "$status_code: $test" >> $TEST_FAILED
        echo "============================================" >> $TEST_FAILED
        cat $ENGINE_TEMP >> $TEST_FAILED
        echo "============================================" >> $TEST_FAILED
        echo >> $TEST_FAILED
        echo >> $TEST_FAILED

        failed=$((failed+1))
    else
        echo "[$tested/$TOTAL] $cmd_line: $PASS"

        echo "$test" >> $TEST_PASSED

        passed=$((passed+1))
    fi

    tested=$((tested+1))
done

rm -f $ENGINE_TEMP

ratio=$(echo $passed*100/$TOTAL | bc)

if [ "$IS_SNAPSHOT" == true ]
then
    ENGINE_ARGS="--snapshot $ENGINE_ARGS"
fi

echo "[summary] ${ENGINE#$ROOT_DIR} $ENGINE_ARGS ${TESTS#$ROOT_DIR}: $passed PASS, $failed FAIL, $TOTAL total, $ratio% success"

if [ $failed -ne 0 ]
then
    echo "$0: see $TEST_FAILED for details about failures"
    exit 1
fi

exit 0
