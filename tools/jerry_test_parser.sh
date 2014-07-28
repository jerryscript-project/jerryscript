# Copyright 2014 Samsung Electronics Co., Ltd.
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

#!/bin/bash

TIMEOUT=3

START_DIR=`pwd`

ENGINE=$START_DIR/$1
OUT_DIR=$2


cd $OUT_DIR

JS_FILES=js.files
JERRY_ERROR=jerry.error

rm -f $JS_FILES $JERRY_ERROR

find $START_DIR -name *.js -print > $JS_FILES
total=$(cat $JS_FILES | wc -l)

tested=0
failed=0
passed=0

exec 2>/dev/null 3>/dev/stderr

echo "  Passed / Failed / Tested / Total / Percent"

for test in `cat $JS_FILES`
do
    percent=$(echo $tested*100/$total | bc)

    ( ulimit -t $TIMEOUT;
      ${ENGINE} ${test} --parse-only > /dev/null;
      exit $? );
    status_code=$?

    printf "\r\e[2K[ %6d / %6d / %6d / %5d / %3d%%    ]" ${passed} ${failed} ${tested} ${total} ${percent}

    if [ $status_code -ne 0 ]
    then
        echo "$status_code: ${test}" >> $JERRY_ERROR

        failed=$((failed+1))
    else
        passed=$((passed+1))
    fi

    tested=$((tested+1))
done

exec 2>&3 2> /dev/null

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
# FIXME: When all tests will pass
   exit 0;
fi

exit 0
