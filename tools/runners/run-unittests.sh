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

DIR="$1"
shift

OPTION_SILENT=disable
while (( "$#" ))
do
  if [ "$1" = "--silent" ]
  then
    OPTION_SILENT=enable
  fi

  shift
done

rm -f $DIR/unit_tests_run.log

UNITTESTS=$(ls $DIR/unit-*)

for unit_test in $UNITTESTS;
do
  [ $OPTION_SILENT = "enable" ] || echo -n "Running $unit_test... ";

  $unit_test >&$DIR/unit_tests_run.log.tmp;
  status_code=$?
  cat $DIR/unit_tests_run.log.tmp >> $DIR/unit_tests_run.log
  rm $DIR/unit_tests_run.log.tmp

  if [ $status_code -eq 0 ];
  then
    [ $OPTION_SILENT = "enable" ] || echo OK;
  else
    [ $OPTION_SILENT = "enable" ] || echo FAILED;
    exit 1;
  fi;
done
