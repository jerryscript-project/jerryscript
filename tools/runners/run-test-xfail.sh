#!/bin/bash

# Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

TIMEOUT=${TIMEOUT:=30}

START_DIR=`pwd`

ENGINE=$START_DIR/$1
shift

OUT_DIR=$1
shift

ERROR_CODE=$1
shift

TESTS=$START_DIR/$1/fail/$ERROR_CODE
shift

ECHO_PROGRESS=1
JERRY_ARGS=
while (( "$#" ))
do
  if [ "$1" = "--parse-only" ]
  then
    JERRY_ARGS="$JERRY_ARGS $1"
  fi
  if [ "$1" = "--output-to-log" ]
  then
    exec 1>$OUT_DIR/jerry_test.log
    ECHO_PROGRESS=0
  fi
  shift
done

if [ ! -x $ENGINE ]
then
  echo \"$ENGINE\" is not an executable file
fi

if [ ! -d $OUT_DIR ]
then
  mkdir -p $OUT_DIR
fi
cd $OUT_DIR

JS_FILES=js.fail.files
JERRY_ERROR=jerry.error
JERRY_OK=jerry.passed

rm -f $JS_FILES $JERRY_ERROR

if [ -d $TESTS ];
then
 find $TESTS -name [^N]*.js -print | sort > $JS_FILES
else
 if [ -f $TESTS ];
 then
  cp $TESTS $JS_FILES
 else
  exit 1
 fi;
fi;
total=$(cat $JS_FILES | wc -l)

tested=0
failed=0
passed=0

JERRY_TEMP=jerry.tmp

echo "  Passed / Failed / Tested / Total / Percent"

for test in `cat $JS_FILES`
do
    percent=$(echo $tested*100/$total | bc)

    ( ulimit -t $TIMEOUT; ${ENGINE} ${test} ${JERRY_ARGS} >&$JERRY_TEMP; exit $? );
    status_code=$?

    if [ $ECHO_PROGRESS -eq 1 ]
    then
      printf "\r\e[2K[ %6d / %6d / %6d / %5d / %3d%%    ]" ${passed} ${failed} ${tested} ${total} ${percent}
    fi

    if [ $status_code -ne $ERROR_CODE ]
    then
        echo "$status_code: ${test}" >> $JERRY_ERROR
        echo "============================================" >> $JERRY_ERROR
        cat $JERRY_TEMP >> $JERRY_ERROR
        echo "============================================" >> $JERRY_ERROR
        echo >> $JERRY_ERROR
        echo >> $JERRY_ERROR

        failed=$((failed+1))
    else
        echo "${test}" >> $JERRY_OK
        passed=$((passed+1))
    fi

    tested=$((tested+1))
done

rm $JERRY_TEMP

printf "\r\e[2K[ %6d / %6d / %6d / %5d / %3d%%    ]\n" ${passed} ${failed} ${tested} ${total} ${percent}

ratio=$(echo $passed*100/$total | bc)

echo ==========================
echo "Number of tests passed:   ${passed}"
echo "Number of tests failed:   ${failed}"
echo --------------------------
echo "Total number of tests:    ${total}"
echo "Passed:                   ${ratio}%"

if [ ${failed} -ne 0 ]
then
   echo "See $JERRY_ERROR for details about failures"
   exit 1;
fi

exit 0
