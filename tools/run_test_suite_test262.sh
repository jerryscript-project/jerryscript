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

TARGET=$1
PARSE=$2

TEST_SUITE_PATH="./tests/test262/test"
STA_JS="$TEST_SUITE_PATH/harness/sta-jerry.js"

for chapter in `ls $TEST_SUITE_PATH/suite`;
do
  rm -f jerry.error."$chapter"

  for test in `find $TEST_SUITE_PATH/suite/$chapter -name *.js`
  do
    grep "\* @negative" $test 2>&1 >/dev/null
    negative=$?

    cmd="./out/$TARGET/jerry $PARSE $test $STA_JS"
    output=`$cmd 2>&1`
    status=$?

    if [[ $status -eq 0 && $negative -eq 0 || $status -ne 0 && $negative -ne 0 ]]
    then
      (echo "====================";
       echo "$cmd failed: $status";
       echo "---------------------";
       echo $output;
       echo;) >> jerry.error."$chapter"
    fi   
  done
done
