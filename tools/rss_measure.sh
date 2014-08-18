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

RSS_OUTPUT="1"
PSS_OUTPUT="1"
SHARE_OUTPUT="1"
SHARECLN_OUTPUT="1"
SHAREDRT_OUTPUT="1"
RSS_SHARE_OUTPUT="1"
PRIVCLEAN_OUTPUT="1"
PRIVDIRTY_OUTPUT="1"

function run_test()
{
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

    PRIVCLEAN_SUM=`cat /proc/$PID/smaps 2>/dev/null | grep Private_Clean | awk '{sum += $2;} END {print sum;}'`
      [ "$PRIVCLEAN_SUM" != "" ] || break;

    PRIVDIRTY_SUM=`cat /proc/$PID/smaps 2>/dev/null | grep Private_Dirty | awk '{sum += $2;} END {print sum;}'`
      [ "$PRIVDIRTY_SUM" != "" ] || break;
      
    SHARECLN_SUM=`cat /proc/$PID/smaps 2>/dev/null | grep Shared_Clean | awk '{sum += $2;} END {print sum;}'`
      [ "$SHARECLN_SUM" != "" ] || break;

    SHAREDRT_SUM=`cat /proc/$PID/smaps 2>/dev/null | grep Shared_Dirty | awk '{sum += $2;} END {print sum;}'`
      [ "$SHAREDRT_SUM" != "" ] || break;

    RSS_SHARE_SUM=$(($RSS_SUM - $SHARE_SUM))

    sleep $SLEEP

    RSS_OUTPUT="$RSS_OUTPUT $RSS_SUM\n"
    PSS_OUTPUT="$PSS_OUTPUT $PSS_SUM\n"
    SHARE_OUTPUT="$SHARE_OUTPUT $SHARE_SUM\n"
    RSS_SHARE_OUTPUT="$RSS_SHARE_OUTPUT $RSS_SHARE_SUM\n"
    PRIVCLEAN_OUTPUT="$PRIVCLEAN_OUTPUT $PRIVCLEAN_SUM\n"
    PRIVDIRTY_OUTPUT="$PRIVDIRTY_OUTPUT $PRIVDIRTY_SUM\n"
    SHARECLN_OUTPUT="$SHARECLN_OUTPUT $SHARECLN_SUM\n"
    SHAREDRT_OUTPUT="$SHAREDRT_OUTPUT $SHAREDRT_SUM\n"

    done
}

START=$(date +%s.%N)

for i in 1 2 3 4 5
do
  run_test
done

FINISH=$(date +%s.%N)
EXEC_TIME=$(echo "$FINISH - $START" | bc)

echo

if [ "$RAW_OUTPUT" != "" ];
then
     echo -e $RSS_OUTPUT;
     echo -e $PSS_OUTPUT;
     echo -e $SHARE_OUTPUT;
fi;

if [ "$RSS_OUTPUT" == "1" ]
then
    echo ===================
    echo "Test failed."
    echo ===================
    exit 1
fi;

TIME=$(echo "scale=3;$EXEC_TIME / 1.0" | bc )
AVG_TIME=$(echo "scale=3;$EXEC_TIME / 5" | bc )

echo ===================
echo -e $RSS_OUTPUT | awk '{ if ($1 != "") { sum += $1; n += 1; if ($1 > max) { max = $1; } } } END { printf "Rss average:\t\t%d Kb\tRss max: %d Kb\n", sum / n, max; }'
echo -e $PSS_OUTPUT | awk '{ if ($1 != "") { sum += $1; n += 1; if ($1 > max) { max = $1; } } } END { printf "Pss average:\t\t%d Kb\tPss max: %d Kb\n", sum / n, max; }'
echo -e $SHARE_OUTPUT | awk '{ if ($1 != "") { sum += $1; n += 1; if ($1 > max) { max = $1; } } } END { printf "Share average:\t\t%d Kb\tShare max: %d Kb\n", sum / n, max; }'
echo -e $RSS_SHARE_OUTPUT | awk '{ if ($1 != "") { sum += $1; n += 1; if ($1 > max) { max = $1; } } } END { printf "Rss - Share average:\t%d Kb\tRss - Share max: %d Kb\n", sum / n, max; }'
echo -e $PRIVCLEAN_OUTPUT | awk '{ if ($1 != "") { sum += $1; n += 1; if ($1 > max) { max = $1; } } } END { printf "Private_Clean average:\t%d Kb\tPrivate_Clean max: %d Kb\n", sum / n, max; }'
echo -e $PRIVDIRTY_OUTPUT | awk '{ if ($1 != "") { sum += $1; n += 1; if ($1 > max) { max = $1; } } } END { printf "Private_Dirty average:\t%d Kb\tPrivate_Dirty max: %d Kb\n", sum / n, max; }'
echo -e $SHARECLN_OUTPUT | awk '{ if ($1 != "") { sum += $1; n += 1; if ($1 > max) { max = $1; } } } END { printf "Shared_Clean average:\t%d Kb\tShared_Clean max: %d Kb\n", sum / n, max; }'
echo -e $SHAREDRT_OUTPUT | awk '{ if ($1 != "") { sum += $1; n += 1; if ($1 > max) { max = $1; } } } END { printf "Shared_Dirty average:\t%d Kb\tShared_Dirty max: %d Kb\n", sum / n, max; }'
echo -e "---"
echo -e "Exec time / average:\t$TIME / $AVG_TIME secs"
echo ===================
