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

RSS_OUTPUT=
PSS_OUTPUT=
SHARE_OUTPUT=

START=$(date +%s.%N)

$JERRY $TEST &
PID=$!

while true;
do
RSS_SUM=`cat /proc/$PID/smaps 2>/dev/null | grep Rss | awk '{sum += $2;} END {print sum;}'`
[ "$RSS_SUM" != "" ] || break;

PSS_SUM=`cat /proc/$PID/smaps 2>/dev/null | grep Pss | awk '{sum += $2;} END {print sum;}'`
[ "$PSS_SUM" != "" ] || break;

SHARE_SUM=`cat /proc/$PID/smaps 2>/dev/null | grep Share | awk '{sum += $2;} END {print sum;}'`
[ "$SHARE_SUM" != "" ] || break;

sleep $SLEEP

RSS_OUTPUT="$RSS_OUTPUT $RSS_SUM\n"
PSS_OUTPUT="$PSS_OUTPUT $PSS_SUM\n"
SHARE_OUTPUT="$SHARE_OUTPUT $SHARE_SUM\n"
done

FINISH=$(date +%s.%N)
EXEC_TIME=$(echo "$FINISH - $START" | bc)

if [ "$RAW_OUTPUT" != "" ];
then
  echo -e $RSS_OUTPUT;
  echo -e $PSS_OUTPUT;
  echo -e $SHARE_OUTPUT;
fi;

echo
echo ===================
echo -e $RSS_OUTPUT | awk '{ if ($1 != "") { sum += $1; n += 1; if ($1 > max) { max = $1; } } } END { printf "Rss average:\t\t%f Kb\tRss max: %d Kb\n", sum / n, max; }'
echo -e $PSS_OUTPUT | awk '{ if ($1 != "") { sum += $1; n += 1; if ($1 > max) { max = $1; } } } END { printf "Pss average:\t\t%f Kb\tPss max: %d Kb\n", sum / n, max; }'
echo -e $SHARE_OUTPUT | awk '{ if ($1 != "") { sum += $1; n += 1; if ($1 > max) { max = $1; } } } END { printf "Share average:\t\t%f Kb\tShare max: %d Kb\n", sum / n, max; }'
echo -e "---"
echo -e "Exec time:\t\t"$EXEC_TIME "secs"
echo ===================
