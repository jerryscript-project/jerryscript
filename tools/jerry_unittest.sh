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

DIR="$1"

rm -f $DIR/unit_tests_run.log

UNITTESTS=$(ls $DIR)

for unit_test in $UNITTESTS;
do
  echo -n "Running $unit_test... ";

  $DIR/$unit_test 2>&1 >> $DIR/unit_tests_run.log;
  if [ $? -eq 0 ];
  then
    echo OK;
  else
    echo FAILED;
    exit 1;
  fi;
done

