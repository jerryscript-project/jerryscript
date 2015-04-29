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

TARGET=$1
shift

PARSE_ONLY=""
VERBOSE_REPORT=0
TIMEOUT=5

function usage() {
  echo "Usage: $0 {path to engine} [-p] [-v] [-t {timeout in seconds}]" >&2
  echo "  -p - parser only mode" >&2
  echo "  -v - verbose report mode" >&2
  echo "  -t - timeout for each test" >&2

  exit 1
}

[ -x "$TARGET" ] || usage

while getopts pvt: OPTION; do
    case "$OPTION" in
      p)
        PARSE_ONLY="--parse-only" ;;
      v)
        VERBOSE_REPORT=1 ;;
      t)
        TIMEOUT="$OPTARG"; [ "$TIMEOUT" -eq "$TIMEOUT" ] || usage;;
      [?])
        usage ;;
    esac
done

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

[ $VERBOSE_REPORT -eq 0 ] || rm -f jerry.error.*
rm -f test262.report

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
  output=`echo "$output" | sed -e "s/^/ /"`

  if [[ $status -eq 0 && $negative -eq 0 || $status -ne 0 && $negative -ne 0 ]];
  then
    (echo "====================";
     echo "Run \"$cmd\" failed: $status";
     echo "---------------------";
     echo "$output";
     echo;) >> $tmp_dir/jerry.error."$chapter"."$test_id";
  else
    (echo "====================";
     echo "Run \"$cmd\" successful";
     echo "---------------------";
     echo "$output";
     echo;) >> $tmp_dir/jerry.ok."$chapter"."$test_id";
  fi;
' "$TARGET $PARSE" $STA_JS $TIMEOUT $TMP_DIR 2>/dev/null;

if [ $VERBOSE_REPORT -eq 1 ]
then
  for CHAPTER in `ls $TEST_SUITE_PATH/suite`;
  do
    cat $TMP_DIR/jerry.error."$CHAPTER".* > jerry.error."$CHAPTER"
  done
fi

cat $TMP_DIR/jerry.ok.* $TMP_DIR/jerry.error.* | grep "^Run" > test262.report

ok_num=`grep "successful$" test262.report | wc -l`
error_num=`grep "Run .* failed: [0-9]*$" test262.report | wc -l`

ok_percent=`echo $ok_num $error_num | awk '{print 100.0 * $1 / ($1 + $2);}'`
error_percent=`echo $ok_num $error_num | awk '{print 100.0 * $2 / ($1 + $2);}'`

echo -e "OK:\t$ok_num ($ok_percent %)"
echo -e "FAIL:\t$error_num ($error_percent %)"
echo
echo -e "Report:\t$PWD/test262.report"

clean_on_exit OK
