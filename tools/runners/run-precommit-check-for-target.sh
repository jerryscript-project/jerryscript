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

ENGINE_PATH="$1"
OUTPUT_PATH="$2"
TESTS_PATH="$3"
TESTS_OPTS="$4"

[ -x $ENGINE_PATH ] || exit 1
[[ -d $TESTS_PATH || -f $TESTS_PATH ]] || exit 1
mkdir -p $OUTPUT_PATH || exit 1

./tools/runners/run-test-pass.sh $ENGINE_PATH $OUTPUT_PATH $TESTS_PATH $TESTS_OPTS --output-to-log; status_code=$?
if [ $status_code -ne 0 ]
then
  echo "$ENGINE_PATH testing failed"
  echo "See log in $OUTPUT_PATH directory for details."
  exit $status_code
fi

if [ -d $TESTS_PATH/fail ]
then
  for error_code in `cd $TESTS_PATH/fail && ls -d [0-9]*`
  do
    ./tools/runners/run-test-xfail.sh $ENGINE_PATH $OUTPUT_PATH $error_code $TESTS_PATH $TESTS_OPTS --output-to-log; status_code=$?

    if [ $status_code -ne 0 ]
    then
      echo "$ENGINE_PATH testing failed"
      echo "See log in $OUTPUT_PATH directory for details."
      exit $status_code
    fi
  done
fi

exit 0
