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

#!/bin/bash

TARGET=$1
PARSE=$2

TIMEOUT=5
TMP_DIR=`mktemp -d --tmpdir=./`

echo $TMP_DIR | grep -q tmp || exit 1

trap clean_on_exit INT

function clean_on_exit() {
  rm -rf $TMP_DIR

  [[ $1 == "OK" ]] || exit 1
}

TEST_SUITE_PATH="./tests/test262/test"
STA_JS="$TEST_SUITE_PATH/harness/sta-jerry.js"
MAX_THREADS=${MAX_CPUS:-32}

rm -f jerry.error.*

find $TEST_SUITE_PATH/suite -name *.js -printf "%p %D%i\0" | xargs -0 -n 1 -P $MAX_THREADS bash -c '
  target=$0
  sta_js_path=$1;
  timeout=$2;
  tmp_dir=$3;
  test=`echo $4 | cut -d " " -f 1`;
  test_id=`echo $4 | cut -d " " -f 2`;
  chapter=`echo $test | cut -d "/" -f 6`;

  grep "\* @negative" $test 2>&1 >/dev/null;
  negative=$?;

  cmd="$target $test $sta_js_path";
  output=`ulimit -t $timeout; $cmd 2>&1`;
  status=$?;

  if [[ $status -eq 0 && $negative -eq 0 || $status -ne 0 && $negative -ne 0 ]];
  then
    (echo "====================";
     echo "$cmd failed: $status";
     echo "---------------------";
     echo $output;
     echo;) >> $tmp_dir/jerry.error."$chapter"."$test_id";
  fi;
' "$TARGET $PARSE" $STA_JS $TIMEOUT $TMP_DIR 2>/dev/null;

for CHAPTER in `ls $TEST_SUITE_PATH/suite`;
do
  cat $TMP_DIR/jerry.error."$CHAPTER".* > jerry.error."$CHAPTER"
done

clean_on_exit OK
