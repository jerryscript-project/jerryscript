#!/bin/bash

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

ITERS="$1"
ENGINE="$2"
BENCHMARK="$3"
PRINT_MIN="$4"
OS=`uname -s | tr [:upper:] [:lower:]`

if [ "$OS" == "darwin" ]
then
  time_regexp='s/user[ 	]*\([0-9]*\)m\([0-9.]*\)s/\1 \2/g'
else
  time_regexp='s/user[ \t]*\([0-9]*\)m\([0-9.]*\)s/\1 \2/g'
fi

perf_values=$( (( for i in `seq 1 1 $ITERS`; do time $ENGINE "$BENCHMARK"; if [ $? -ne 0 ]; then exit 1; fi; done ) 2>&1 ) | \
               grep user | \
               sed "$time_regexp" | \
               awk 'BEGIN { min_v = -1; } { v = $1 * 60 + $2; if (min_v == -1 || v < min_v) { min_v = v; }; s += v; n += 1; } END { print s / n, min_v; }';
               if [ ${PIPESTATUS[0]} -ne 0 ]; then exit 1; fi;);

if [ $? -ne 0 ];
then
  exit 1;
fi;

if [ "$PRINT_MIN" == "-min" ]
then
  echo $perf_values | cut -d ' ' -f 2
else
  echo $perf_values | cut -d ' ' -f 1
fi
