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

OUTPUT=

$JERRY $TEST &
PID=$!

while true;
do
RSS_SUM=`cat /proc/$PID/smaps 2>/dev/null | grep Rss | awk '{sum += $2;} END {print sum;}'`
[ "$RSS_SUM" != "" ] || break;

OUTPUT="$OUTPUT $RSS_SUM\n"
done

if [ "$RAW_OUTPUT" != "" ];
then
  echo -e $OUTPUT;
fi;

echo
echo ===================
echo -e $OUTPUT | awk '{ if ($1 != "") { sum += $1; n += 1; if ($1 > max) { max = $1; } } } END { printf "Rss average: %f Kb, Rss max: %d Kb\n", sum / n, max; }'
echo ===================
