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

NUM_COMMITS=$1
BENCH=./benchmarks/jerry/loop_arithmetics_1kk.js
TARGET=release.linux

trap ctrl_c INT

function ctrl_c() {
  git checkout master >&/dev/null
  exit 1
}

commits_to_push=`git log -$NUM_COMMITS | grep "^commit [0-9a-f]*$" | awk 'BEGIN { s = ""; } { s = $2" "s; } END { print s; }'`

for commit_hash in $commits_to_push
do
  echo "Testing..."
  git log --format=%B -n 1 $commit_hash
  make clean $TARGET
  ./tools/rss_measure.sh ./out/$TARGET/jerry $BENCH
  echo
done

git checkout master >&/dev/null
