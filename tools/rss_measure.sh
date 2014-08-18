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

JERRY=$1
TEST=$2
RAW_OUTPUT=$3
SLEEP=0.3
REPEATS=5

Size_OUT=""
Rss_OUT=""
Pss_OUT=""
Share_OUT=""
Shared_Clean_OUT=""
Shared_Dirty_OUT=""
Private_Clean_OUT=""
Private_Dirty_OUT=""
Swap_OUT=""

function collect_entry()
{
  OUT_NAME="$1_OUT";
  OUT=$OUT_NAME;

  SUM=`cat /proc/$PID/smaps 2>/dev/null | grep $1 | awk '{sum += $2;} END { if (sum != 0) { print sum; }; }'`;
  
  if [ "$SUM" != "" ];
  then
    eval "$OUT"="\"\$$OUT $SUM\\n\"";
  fi;
}

function print_entry()
{
  OUT_NAME="$1_OUT";
  OUT=$OUT_NAME;
  
  eval "echo -e \"\$$OUT\"" | awk -v entry="$1" '{ if ($1 != "") { sum += $1; n += 1; if ($1 > max) { max = $1; } } } END { if (n == 0) { exit; }; printf "%19s:%8d Kb%19s:%8d Kb\n", entry, sum / n, entry, max; }';
}

function run_test()
{
  $JERRY $TEST &
  PID=$!

  while kill -0 "$PID" > /dev/null 2>&1;
  do
    collect_entry Size
    collect_entry Rss
    collect_entry Pss
    collect_entry Share
    collect_entry Shared_Clean
    collect_entry Shared_Dirty
    collect_entry Private_Clean
    collect_entry Private_Dirty
    collect_entry Swap
    
    sleep $SLEEP

  done
}


ITERATIONS=`seq 1 $REPEATS`
START=$(date +%s.%N)

for i in $ITERATIONS
do
  run_test
done

FINISH=$(date +%s.%N)

echo

if [ "$RAW_OUTPUT" != "" ];
then
   echo -e "$Size_OUT";
   echo -e "$Rss_OUT";
   echo -e "$Pss_OUT";
   echo -e "$Share_OUT";
   echo -e "$Shared_Clean_OUT";
   echo -e "$Shared_Dirty_OUT";
   echo -e "$Private_Clean_OUT";
   echo -e "$Private_Dirty_OUT";
   echo -e "$Swap_OUT";
fi;

if [ "$Size_OUT" == "" ]
then
  echo ===================
  echo "Test failed."
  echo ===================
  exit 1
fi;

TIME=$(echo "scale=3;($FINISH - $START) / 1.0" | bc );
AVG_TIME=$(echo "scale=3;$TIME / $REPEATS" | bc );

echo ===================
printf "%24sAVERAGE%28sMAX\n" "" "";
print_entry Size
print_entry Rss
print_entry Pss
print_entry Share
print_entry Shared_Clean
print_entry Shared_Dirty
print_entry Private_Clean
print_entry Private_Dirty
print_entry Swap
echo -e "---"
echo -e "Exec time / average:\t$TIME / $AVG_TIME secs"
echo ===================
