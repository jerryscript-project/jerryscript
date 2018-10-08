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
#       ./tools/runners/run-test-suite.sh ENGINE TESTS [-q] [--skip-list=item1,item2] [--snapshot] ENGINE_ARGS....

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

TERM_NORMAL="\033[0m"
TERM_RED="\033[1;31m"
TERM_GREEN="\033[1;32m"

trap 'exit' SIGINT

VERBOSE=1
if [[ "$1" == "-q" ]]
then
    unset VERBOSE
    shift
fi

if [[ "$1" =~ ^--skip-list=.* ]]
then
    SKIP_LIST=${1#--skip-list=}
    SKIP_LIST=${SKIP_LIST//,/ }
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

    ( cd $TESTS; find . -name "*.js" ) | sort > $TEST_FILES
elif [ -f $TESTS ]
then
    TESTS_DIR=`dirname $TESTS`

    grep -e '.js\s*$' $TESTS | sort > $TEST_FILES
else
    echo "$0: $TESTS: not a test suite"
    exit 1
fi

# Remove the skipped tests from list
for TEST in ${SKIP_LIST}
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

        $TIMEOUT_CMD $TIMEOUT $RUNTIME $SNAPSHOT_TOOL generate -o $SNAPSHOT_TEMP $full_test &> $ENGINE_TEMP
        status_code=$?
        comment=" (generate snapshot)"

        if [ $status_code -eq 0 ]
        then
            test $VERBOSE && printf "[%4d/%4d] %bPASS: %s%s%b\n" "$tested" "$TOTAL" "$TERM_GREEN" "${full_test#$ROOT_DIR}" "$comment" "$TERM_NORMAL"

            $TIMEOUT_CMD $TIMEOUT $RUNTIME $ENGINE $ENGINE_ARGS --exec-snapshot $SNAPSHOT_TEMP &> $ENGINE_TEMP
            status_code=$?
            comment=" (execute snapshot)"
        fi

        rm -f $SNAPSHOT_TEMP
    else
        $TIMEOUT_CMD $TIMEOUT $RUNTIME $ENGINE $ENGINE_ARGS $full_test &> $ENGINE_TEMP
        status_code=$?
        comment=""
    fi

    if [ $status_code -ne $error_code ]
    then
        printf "[%4d/%4d] %bFAIL (%d): %s%s%b\n" "$tested" "$TOTAL" "$TERM_RED" "$status_code" "${full_test#$ROOT_DIR}" "$comment" "$TERM_NORMAL"
        cat $ENGINE_TEMP

        echo "$status_code: $test" >> $TEST_FAILED
        echo "============================================" >> $TEST_FAILED
        cat $ENGINE_TEMP >> $TEST_FAILED
        echo "============================================" >> $TEST_FAILED
        echo >> $TEST_FAILED
        echo >> $TEST_FAILED

        failed=$((failed+1))
    else
        test $VERBOSE && printf "[%4d/%4d] %b%s: %s%s%b\n" "$tested" "$TOTAL" "$TERM_GREEN" "$PASS" "${full_test#$ROOT_DIR}" "$comment" "$TERM_NORMAL"
        echo "$test" >> $TEST_PASSED

        passed=$((passed+1))
    fi

    tested=$((tested+1))
done

rm -f $ENGINE_TEMP

ratio=$(echo $passed*100/$TOTAL | bc)
if [ $passed -eq $TOTAL ]
then
    success_color=$TERM_GREEN
else
    success_color=$TERM_RED
fi

if [ "$IS_SNAPSHOT" == true ]
then
    ENGINE_ARGS="--snapshot $ENGINE_ARGS"
fi

echo -e "\n[summary] ${ENGINE#$ROOT_DIR} $ENGINE_ARGS ${TESTS#$ROOT_DIR}\n"
echo -e "TOTAL: $TOTAL"
echo -e "${TERM_GREEN}PASS: $passed${TERM_NORMAL}"
echo -e "${TERM_RED}FAIL: $failed${TERM_NORMAL}\n"
echo -e "${success_color}Success: $ratio%${TERM_NORMAL}\n"

if [ $failed -ne 0 ]
then
    echo "$0: see $TEST_FAILED for details about failures"
    exit 1
fi

exit 0
